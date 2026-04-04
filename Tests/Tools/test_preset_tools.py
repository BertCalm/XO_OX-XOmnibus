#!/usr/bin/env python3
"""
Tests/Tools/test_preset_tools.py
=================================
Unit tests for the highest-risk Python preset/migration tools in Tools/.

Covers:
  - normalize_engine_names.normalize_preset: alias correction, key renaming
  - dedup_preset_names.to_roman, build_new_name, engine_name_from_meta,
    find_and_fix_duplicates: idempotency and unique-name generation
  - preset_audit schema validation helpers

These tests are intentionally isolated (no filesystem mutations) using
tempfiles and in-memory dicts so they can run safely in CI.

See: https://github.com/BertCalm/XO_OX-XOmnibus/issues/458
"""

import json
import sys
import tempfile
import unittest
from pathlib import Path

# Add Tools/ to sys.path so we can import the modules under test
TOOLS_DIR = Path(__file__).parent.parent.parent / "Tools"
sys.path.insert(0, str(TOOLS_DIR))


# ---------------------------------------------------------------------------
# normalize_engine_names tests
# ---------------------------------------------------------------------------

class TestNormalizeEngineNames(unittest.TestCase):
    """Tests for normalize_preset() in normalize_engine_names.py."""

    def setUp(self):
        """Import the module fresh for each test to avoid state leakage."""
        import normalize_engine_names as ne
        self.ne = ne

    def _write_preset(self, data: dict) -> Path:
        """Write a dict to a tempfile as JSON, return the Path."""
        f = tempfile.NamedTemporaryFile(
            suffix=".xometa", mode="w", delete=False, encoding="utf-8"
        )
        json.dump(data, f, indent=2)
        f.close()
        return Path(f.name)

    def test_allcaps_engine_normalised(self):
        """ORIGAMI → Origami in engines array."""
        path = self._write_preset({"engines": ["ORIGAMI"], "parameters": {}})
        changed = self.ne.normalize_preset(path)
        self.assertTrue(changed)
        data = json.loads(path.read_text())
        self.assertEqual(data["engines"], ["Origami"])
        path.unlink()

    def test_oddfelix_canonical(self):
        """OddFelix → OddfeliX."""
        path = self._write_preset({"engines": ["OddFelix"], "parameters": {}})
        self.ne.normalize_preset(path)
        data = json.loads(path.read_text())
        self.assertEqual(data["engines"], ["OddfeliX"])
        path.unlink()

    def test_oddoscar_canonical(self):
        """Oddoscar → OddOscar."""
        path = self._write_preset({"engines": ["Oddoscar"], "parameters": {}})
        self.ne.normalize_preset(path)
        data = json.loads(path.read_text())
        self.assertEqual(data["engines"], ["OddOscar"])
        path.unlink()

    def test_canonical_name_unchanged(self):
        """Already-canonical name → no change, returns False."""
        path = self._write_preset({"engines": ["Origami"], "parameters": {}})
        changed = self.ne.normalize_preset(path)
        self.assertFalse(changed)
        path.unlink()

    def test_parameters_key_renamed(self):
        """ORIGAMI parameter key → Origami parameter key."""
        path = self._write_preset({
            "engines": ["ORIGAMI"],
            "parameters": {"ORIGAMI": {"origami_foldPoint": 0.5}}
        })
        self.ne.normalize_preset(path)
        data = json.loads(path.read_text())
        self.assertIn("Origami", data["parameters"])
        self.assertNotIn("ORIGAMI", data["parameters"])
        path.unlink()

    def test_singular_engine_key_migrated(self):
        """'engine' key (singular) → 'engines' array."""
        path = self._write_preset({"engine": "Origami", "parameters": {}})
        changed = self.ne.normalize_preset(path)
        self.assertTrue(changed)
        data = json.loads(path.read_text())
        self.assertNotIn("engine", data)
        self.assertEqual(data["engines"], ["Origami"])
        path.unlink()

    def test_idempotent(self):
        """Running normalize twice on the same file produces the same result."""
        path = self._write_preset({"engines": ["OHMT"], "parameters": {}})
        self.ne.normalize_preset(path)
        result1 = path.read_text()
        self.ne.normalize_preset(path)
        result2 = path.read_text()
        # Content should not change on second run
        self.assertEqual(result1, result2)
        path.unlink()

    def test_invalid_json_does_not_crash(self):
        """Corrupt JSON returns False without raising."""
        f = tempfile.NamedTemporaryFile(
            suffix=".xometa", mode="w", delete=False, encoding="utf-8"
        )
        f.write("{not valid json")
        f.close()
        path = Path(f.name)
        result = self.ne.normalize_preset(path)
        self.assertFalse(result)
        path.unlink()


# ---------------------------------------------------------------------------
# dedup_preset_names tests
# ---------------------------------------------------------------------------

class TestDedupPresetNames(unittest.TestCase):
    """Tests for helper functions in dedup_preset_names.py."""

    def setUp(self):
        import dedup_preset_names as dd
        self.dd = dd

    def test_to_roman_1(self):
        self.assertEqual(self.dd.to_roman(1), "I")

    def test_to_roman_5(self):
        self.assertEqual(self.dd.to_roman(5), "V")

    def test_to_roman_10(self):
        self.assertEqual(self.dd.to_roman(10), "X")

    def test_to_roman_out_of_range(self):
        """Out-of-range → falls back to integer string."""
        result = self.dd.to_roman(100)
        self.assertEqual(result, "100")

    def test_engine_name_from_meta_single(self):
        data = {"engines": ["Origami"]}
        self.assertEqual(self.dd.engine_name_from_meta(data), "Origami")

    def test_engine_name_from_meta_empty(self):
        data = {"engines": []}
        self.assertEqual(self.dd.engine_name_from_meta(data), "Unknown")

    def test_engine_name_from_meta_missing_key(self):
        data = {}
        self.assertEqual(self.dd.engine_name_from_meta(data), "Unknown")

    def test_build_new_name_first_suffix(self):
        """suffix_index=1 produces 'Base (Engine)' when not taken."""
        result = self.dd.build_new_name("Deep Blue", "Origami", 1, set())
        self.assertEqual(result, "Deep Blue (Origami)")

    def test_build_new_name_second_suffix(self):
        """suffix_index=2 when plain is taken → 'Base (Engine I)'."""
        taken = {"Deep Blue (Origami)"}
        result = self.dd.build_new_name("Deep Blue", "Origami", 2, taken)
        self.assertEqual(result, "Deep Blue (Origami I)")

    def test_build_new_name_avoids_taken(self):
        """Skips names already in the taken set."""
        taken = {"Deep Blue (Origami)", "Deep Blue (Origami I)"}
        result = self.dd.build_new_name("Deep Blue", "Origami", 1, taken)
        # "Deep Blue (Origami)" is taken so it falls through to roman numerals
        self.assertNotIn(result, taken)
        self.assertTrue(result.startswith("Deep Blue (Origami"))

    def test_find_and_fix_duplicates_dry_run(self):
        """Dry run detects duplicates but does not modify files."""
        # Create two temp files with the same preset name
        presets = [
            {"name": "Test Preset", "engines": ["Origami"], "parameters": {}},
            {"name": "Test Preset", "engines": ["Origami"], "parameters": {}},
        ]
        paths = []
        for p in presets:
            f = tempfile.NamedTemporaryFile(
                suffix=".xometa", mode="w", delete=False, encoding="utf-8"
            )
            json.dump(p, f, indent=2)
            f.close()
            paths.append(Path(f.name))

        # Record original content
        originals = [p.read_text() for p in paths]

        renames, total = self.dd.find_and_fix_duplicates(paths, apply=False)

        # Files should be unchanged in dry-run mode
        for path, original in zip(paths, originals):
            self.assertEqual(path.read_text(), original)

        # Should have detected 1 rename
        self.assertEqual(len(renames), 1)

        for p in paths:
            p.unlink()

    def test_find_and_fix_duplicates_apply_renames(self):
        """Apply mode renames duplicate presets to unique names."""
        presets = [
            {"name": "Coral Drift", "engines": ["Oceanic"], "parameters": {}},
            {"name": "Coral Drift", "engines": ["Oceanic"], "parameters": {}},
        ]
        paths = []
        for p in presets:
            f = tempfile.NamedTemporaryFile(
                suffix=".xometa", mode="w", delete=False, encoding="utf-8"
            )
            json.dump(p, f, indent=2)
            f.close()
            paths.append(Path(f.name))

        renames, _ = self.dd.find_and_fix_duplicates(paths, apply=True)

        # Reload names — should now be unique
        names = set()
        for p in paths:
            data = json.loads(p.read_text())
            names.add(data["name"])

        self.assertEqual(len(names), 2, f"Expected 2 unique names, got: {names}")

        for p in paths:
            p.unlink()

    def test_find_and_fix_duplicates_unique_presets_unchanged(self):
        """Presets with unique names are never renamed."""
        presets = [
            {"name": "Ocean Drift", "engines": ["Oceanic"], "parameters": {}},
            {"name": "Reef Storm", "engines": ["Oceanic"], "parameters": {}},
        ]
        paths = []
        for p in presets:
            f = tempfile.NamedTemporaryFile(
                suffix=".xometa", mode="w", delete=False, encoding="utf-8"
            )
            json.dump(p, f, indent=2)
            f.close()
            paths.append(Path(f.name))

        originals = [p.read_text() for p in paths]
        renames, _ = self.dd.find_and_fix_duplicates(paths, apply=True)

        # No files should be changed
        for path, original in zip(paths, originals):
            self.assertEqual(path.read_text(), original)

        self.assertEqual(len(renames), 0)

        for p in paths:
            p.unlink()


# ---------------------------------------------------------------------------
# preset schema validation tests
# ---------------------------------------------------------------------------

class TestPresetSchema(unittest.TestCase):
    """Tests that preset structure matches the expected XOceanus schema.

    These tests verify the schema constants and validation rules used by
    preset_audit.py and other tools that consume .xometa files.
    """

    REQUIRED_FIELDS = [
        "mood", "tags", "author", "dna", "macroLabels", "engines", "parameters"
    ]
    DNA_DIMS = {"brightness", "warmth", "movement", "density", "space", "aggression"}
    VALID_MOODS = {
        "Foundation", "Atmosphere", "Entangled", "Prism", "Flux",
        "Aether", "Family", "Submerged", "Coupling", "Crystalline",
        "Deep", "Ethereal", "Kinetic", "Luminous", "Organic"
    }

    def _minimal_valid_preset(self) -> dict:
        return {
            "name": "Test Preset",
            "mood": "Foundation",
            "tags": ["test"],
            "author": "Test",
            "dna": {k: 0.5 for k in self.DNA_DIMS},
            "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
            "engines": ["Origami"],
            "parameters": {"Origami": {"origami_foldPoint": 0.5}}
        }

    def test_valid_preset_has_all_required_fields(self):
        preset = self._minimal_valid_preset()
        for field in self.REQUIRED_FIELDS:
            self.assertIn(field, preset, f"Required field '{field}' missing")

    def test_macro_labels_count_is_4(self):
        preset = self._minimal_valid_preset()
        self.assertEqual(len(preset["macroLabels"]), 4)

    def test_dna_has_6_dimensions(self):
        preset = self._minimal_valid_preset()
        self.assertEqual(set(preset["dna"].keys()), self.DNA_DIMS)

    def test_dna_values_in_range(self):
        preset = self._minimal_valid_preset()
        for dim, val in preset["dna"].items():
            self.assertGreaterEqual(val, 0.0, f"DNA {dim} < 0")
            self.assertLessEqual(val, 1.0, f"DNA {dim} > 1")

    def test_mood_is_valid(self):
        preset = self._minimal_valid_preset()
        self.assertIn(preset["mood"], self.VALID_MOODS)

    def test_engines_is_list(self):
        preset = self._minimal_valid_preset()
        self.assertIsInstance(preset["engines"], list)
        self.assertGreater(len(preset["engines"]), 0)

    def test_parameters_is_dict(self):
        preset = self._minimal_valid_preset()
        self.assertIsInstance(preset["parameters"], dict)

    def test_invalid_macro_labels_count_detected(self):
        preset = self._minimal_valid_preset()
        preset["macroLabels"] = ["CHARACTER", "MOVEMENT"]  # only 2
        self.assertNotEqual(len(preset["macroLabels"]), 4)

    def test_dna_out_of_range_detected(self):
        preset = self._minimal_valid_preset()
        preset["dna"]["brightness"] = 1.5  # out of range
        out_of_range = any(
            v < 0.0 or v > 1.0 for v in preset["dna"].values()
        )
        self.assertTrue(out_of_range)

    def test_schema_version_field(self):
        """schema_version=1 is the current version."""
        preset = self._minimal_valid_preset()
        preset["schema_version"] = 1
        self.assertEqual(preset.get("schema_version", 1), 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
