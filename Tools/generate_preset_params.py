#!/usr/bin/env python3
"""Generate DSP parameter values for all factory .xocmeta presets.

Reads each .xocmeta, generates musically appropriate parameters based on
the preset's category, tags, description, and metadata fields, then writes
the parameters back into the .xocmeta JSON.
"""

import json
import os
import hashlib

PRESETS_DIR = os.path.join(os.path.dirname(__file__), '..', 'Presets', 'Factory')

# Parameter ranges from createParameterLayout()
PARAM_RANGES = {
    # Engine X
    'xOscMode': {'type': 'choice', 'options': [0, 1, 2]},  # 0=Sine+Noise, 1=FM, 2=Karplus-Strong
    'xSnap': {'min': 0.0, 'max': 1.0, 'default': 0.4},
    'xDecay': {'min': 0.0, 'max': 8.0, 'default': 0.5},
    'xFilterCutoff': {'min': 20.0, 'max': 20000.0, 'default': 2000.0},
    'xFilterReso': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    'xDetune': {'min': 0.0, 'max': 50.0, 'default': 10.0},
    'xLevel': {'min': 0.0, 'max': 1.0, 'default': 0.8},
    'xPitchLock': {'type': 'bool', 'default': 0},
    'xUnison': {'type': 'choice', 'options': [0, 1, 2]},  # 0="1", 1="2", 2="4"
    'xPolyphony': {'type': 'choice', 'options': [0, 1, 2, 3]},  # 0="1", 1="2", 2="4", 3="8"
    # Engine O
    'oMorph': {'min': 0.0, 'max': 3.0, 'default': 0.5},
    'oBloom': {'min': 0.001, 'max': 10.0, 'default': 1.25},
    'oDecay': {'min': 0.01, 'max': 8.0, 'default': 2.0},
    'oSustain': {'min': 0.0, 'max': 1.0, 'default': 0.7},
    'oRelease': {'min': 0.01, 'max': 10.0, 'default': 1.5},
    'oFilterCutoff': {'min': 20.0, 'max': 20000.0, 'default': 1200.0},
    'oFilterReso': {'min': 0.0, 'max': 1.0, 'default': 0.4},
    'oDrift': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    'oSub': {'min': 0.0, 'max': 1.0, 'default': 0.5},
    'oDetune': {'min': 0.0, 'max': 50.0, 'default': 12.0},
    'oLevel': {'min': 0.0, 'max': 1.0, 'default': 0.8},
    'oPolyphony': {'type': 'choice', 'options': [0, 1, 2, 3, 4]},  # 0-4 = 1,2,4,8,16
    # Coupling
    'couplingAmount': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    # Master
    'masterBalance': {'min': -1.0, 'max': 1.0, 'default': 0.0},
    # Macros
    'macro1': {'min': 0.0, 'max': 1.0, 'default': 0.5},
    'macro2': {'min': 0.0, 'max': 1.0, 'default': 0.5},
    'macro3': {'min': 0.0, 'max': 1.0, 'default': 0.5},
    'macro4': {'min': 0.0, 'max': 1.0, 'default': 0.5},
    # Effects enables
    'delayEnabled': {'type': 'bool', 'default': 1},
    'reverbEnabled': {'type': 'bool', 'default': 1},
    'phaserEnabled': {'type': 'bool', 'default': 0},
    'lofiEnabled': {'type': 'bool', 'default': 0},
    'compressorEnabled': {'type': 'bool', 'default': 1},
    # Delay
    'delayTime': {'min': 0.01, 'max': 2.0, 'default': 0.375},
    'delayFeedback': {'min': 0.0, 'max': 0.95, 'default': 0.5},
    'delayMix': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    'delayWobble': {'min': 0.0, 'max': 1.0, 'default': 0.2},
    # Reverb
    'reverbDecay': {'min': 0.1, 'max': 0.99, 'default': 0.7},
    'reverbDamping': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    'reverbSize': {'min': 0.1, 'max': 2.0, 'default': 1.0},
    'reverbMix': {'min': 0.0, 'max': 1.0, 'default': 0.25},
    # Phaser
    'phaserRate': {'min': 0.01, 'max': 10.0, 'default': 0.5},
    'phaserDepth': {'min': 0.0, 'max': 1.0, 'default': 0.5},
    'phaserMix': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    # Lo-Fi
    'lofiBitDepth': {'min': 2.0, 'max': 16.0, 'default': 12.0},
    'lofiRateReduce': {'min': 1.0, 'max': 64.0, 'default': 1.0},
    'lofiWowFlutter': {'min': 0.0, 'max': 1.0, 'default': 0.0},
    'lofiMix': {'min': 0.0, 'max': 1.0, 'default': 0.3},
    # Compressor
    'compThreshold': {'min': -40.0, 'max': 0.0, 'default': -12.0},
    'compRatio': {'min': 1.0, 'max': 20.0, 'default': 4.0},
    'compAttack': {'min': 0.1, 'max': 100.0, 'default': 10.0},
    'compRelease': {'min': 1.0, 'max': 1000.0, 'default': 100.0},
    # Sequencer
    'seqPlaying': {'type': 'bool', 'default': 0},
    'seqTempo': {'min': 40.0, 'max': 300.0, 'default': 120.0},
    'seqSwing': {'min': 0.0, 'max': 1.0, 'default': 0.0},
}


def deterministic_hash(name, salt=''):
    """Get a deterministic float 0-1 from a preset name."""
    h = hashlib.md5((name + salt).encode()).hexdigest()
    return int(h[:8], 16) / 0xFFFFFFFF


def clamp(val, lo, hi):
    return max(lo, min(hi, val))


def generate_sequencer_pattern(name, tempo, num_steps=16, style='basic'):
    """Generate a sequencer pattern based on style."""
    h = deterministic_hash(name, 'seq')
    tracks = []

    if style == 'none':
        return None

    # Track 0: kick/bass pattern
    steps_t0 = []
    if style in ('four_on_floor', 'techno'):
        for i in range(num_steps):
            if i % 4 == 0:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.9})
    elif style in ('halftime', 'dub'):
        for i in range(num_steps):
            if i % 8 == 0:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.85})
            elif i == 10:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.6})
    elif style == 'breakbeat':
        pattern = [0, 4, 6, 10, 12]
        for i in pattern:
            if i < num_steps:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.85 + deterministic_hash(name, f's0_{i}') * 0.15})
    elif style == 'dnb':
        pattern = [0, 6, 8, 14]
        for i in pattern:
            if i < num_steps:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.9})
    elif style == 'triplet':
        for i in range(0, num_steps, 3):
            steps_t0.append({'index': i, 'note': 36, 'velocity': 0.85})
    elif style == 'sparse':
        for i in range(num_steps):
            if deterministic_hash(name, f'sp0_{i}') > 0.7:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.5 + deterministic_hash(name, f'sv0_{i}') * 0.4})
    else:  # basic
        for i in range(num_steps):
            if i % 4 == 0:
                steps_t0.append({'index': i, 'note': 36, 'velocity': 0.85})

    tracks.append({'length': num_steps, 'muted': False, 'steps': steps_t0})

    # Track 1: snare/clap
    steps_t1 = []
    if style in ('four_on_floor', 'techno', 'basic'):
        for i in range(num_steps):
            if i % 8 == 4:
                steps_t1.append({'index': i, 'note': 38, 'velocity': 0.8})
    elif style == 'breakbeat':
        pattern = [4, 12]
        for i in pattern:
            if i < num_steps:
                steps_t1.append({'index': i, 'note': 38, 'velocity': 0.8})
    elif style == 'dnb':
        pattern = [4, 12]
        for i in pattern:
            if i < num_steps:
                steps_t1.append({'index': i, 'note': 38, 'velocity': 0.85})
    elif style in ('dub', 'halftime'):
        steps_t1.append({'index': 4, 'note': 38, 'velocity': 0.75})
        steps_t1.append({'index': 12, 'note': 38, 'velocity': 0.7})
    tracks.append({'length': num_steps, 'muted': False, 'steps': steps_t1})

    # Track 2: hi-hat
    steps_t2 = []
    if style in ('four_on_floor', 'techno'):
        for i in range(num_steps):
            if i % 2 == 0:
                vel = 0.6 if i % 4 == 0 else 0.4
                steps_t2.append({'index': i, 'note': 42, 'velocity': vel})
    elif style == 'dnb':
        for i in range(num_steps):
            if i % 2 == 0:
                steps_t2.append({'index': i, 'note': 42, 'velocity': 0.5 + deterministic_hash(name, f'hh_{i}') * 0.3})
    elif style == 'breakbeat':
        for i in range(num_steps):
            if deterministic_hash(name, f'bh_{i}') > 0.4:
                steps_t2.append({'index': i, 'note': 42, 'velocity': 0.3 + deterministic_hash(name, f'bhv_{i}') * 0.5})
    elif style in ('dub', 'halftime'):
        for i in range(0, num_steps, 2):
            steps_t2.append({'index': i, 'note': 42, 'velocity': 0.45})
    tracks.append({'length': num_steps, 'muted': False, 'steps': steps_t2})

    # Track 3: melodic/accent (use preset-specific notes)
    steps_t3 = []
    base_note = 48 + int(deterministic_hash(name, 'note') * 24)
    scale = [0, 2, 3, 5, 7, 10, 12]  # minor pentatonic-ish
    for i in range(num_steps):
        if deterministic_hash(name, f'mel_{i}') > 0.75:
            note_offset = scale[int(deterministic_hash(name, f'mn_{i}') * len(scale))]
            steps_t3.append({
                'index': i,
                'note': base_note + note_offset,
                'velocity': 0.5 + deterministic_hash(name, f'mv_{i}') * 0.4
            })
    tracks.append({'length': num_steps, 'muted': style in ('sparse',), 'steps': steps_t3})

    return {'tracks': tracks}


def generate_params(meta):
    """Generate DSP parameters for a preset based on its metadata."""
    name = meta['name']
    category = meta['category']
    tags = meta.get('tags', [])
    desc = meta.get('description', '').lower()
    balance = meta.get('engineBalance', 'Balanced')
    coupling_intensity = meta.get('couplingIntensity', 'None')
    tempo = meta.get('tempo', None)
    has_seq = meta.get('sequencerPattern', False)

    # Deterministic seed for this preset
    h = deterministic_hash(name)
    h2 = deterministic_hash(name, 'v2')
    h3 = deterministic_hash(name, 'v3')

    params = {}

    # ---- Coupling Amount ----
    coupling_map = {'None': 0.0, 'Subtle': 0.2, 'Moderate': 0.5, 'Deep': 0.8}
    base_coupling = coupling_map.get(coupling_intensity, 0.3)
    params['couplingAmount'] = clamp(base_coupling + (h - 0.5) * 0.15, 0.0, 1.0)

    # ---- Master Balance ----
    if balance == 'X-dominant':
        params['masterBalance'] = clamp(-0.3 - h * 0.3, -1.0, 1.0)
    elif balance == 'O-dominant':
        params['masterBalance'] = clamp(0.3 + h * 0.3, -1.0, 1.0)
    elif balance == 'Coupled':
        params['masterBalance'] = (h - 0.5) * 0.2
    else:
        params['masterBalance'] = (h - 0.5) * 0.15

    # ---- Engine X: Osc Mode ----
    if any(t in tags for t in ['FM', 'metallic', 'bell', 'fm']):
        params['xOscMode'] = 1
    elif any(t in tags for t in ['karplus-strong', 'wire', 'pluck', 'string']):
        params['xOscMode'] = 2
    elif any(t in tags for t in ['noise', 'lo-fi', 'hiss']):
        params['xOscMode'] = 0
    else:
        params['xOscMode'] = int(h * 2.99)

    # ---- Engine X parameters ----
    # Snap (transient sharpness)
    if any(t in tags for t in ['sharp', 'click', 'transient', 'snap', 'bright']):
        params['xSnap'] = clamp(0.6 + h * 0.4, 0.0, 1.0)
    elif any(t in tags for t in ['soft', 'warm', 'gentle', 'smooth']):
        params['xSnap'] = clamp(0.1 + h * 0.2, 0.0, 1.0)
    elif category == 'Grounded':
        params['xSnap'] = clamp(0.4 + h * 0.4, 0.0, 1.0)
    elif category == 'Floating':
        params['xSnap'] = clamp(0.1 + h * 0.3, 0.0, 1.0)
    elif category == 'Deep Space':
        params['xSnap'] = clamp(0.05 + h * 0.25, 0.0, 1.0)
    else:
        params['xSnap'] = clamp(0.3 + h * 0.4, 0.0, 1.0)

    # Decay
    if category == 'Grounded':
        params['xDecay'] = clamp(0.1 + h * 1.5, 0.0, 8.0)
    elif category == 'Floating':
        params['xDecay'] = clamp(1.5 + h * 4.0, 0.0, 8.0)
    elif category == 'Deep Space':
        params['xDecay'] = clamp(2.0 + h * 5.0, 0.0, 8.0)
    else:  # Entangled
        params['xDecay'] = clamp(0.3 + h * 3.0, 0.0, 8.0)

    if any(t in tags for t in ['short', 'staccato', 'click']):
        params['xDecay'] = clamp(params['xDecay'] * 0.3, 0.0, 8.0)
    elif any(t in tags for t in ['long', 'sustained', 'infinite']):
        params['xDecay'] = clamp(params['xDecay'] * 2.0, 0.0, 8.0)

    # Filter
    if any(t in tags for t in ['dark', 'muffled', 'deep', 'sub']):
        params['xFilterCutoff'] = clamp(200 + h * 1500, 20, 20000)
    elif any(t in tags for t in ['bright', 'crisp', 'sharp', 'metallic']):
        params['xFilterCutoff'] = clamp(4000 + h * 12000, 20, 20000)
    elif category == 'Deep Space':
        params['xFilterCutoff'] = clamp(800 + h * 6000, 20, 20000)
    elif category == 'Floating':
        params['xFilterCutoff'] = clamp(1000 + h * 5000, 20, 20000)
    elif category == 'Grounded':
        params['xFilterCutoff'] = clamp(1500 + h * 8000, 20, 20000)
    else:
        params['xFilterCutoff'] = clamp(1200 + h * 6000, 20, 20000)

    if any(t in tags for t in ['resonant', 'squelchy', 'acid']):
        params['xFilterReso'] = clamp(0.6 + h * 0.35, 0.0, 1.0)
    elif any(t in tags for t in ['clean', 'pure', 'smooth']):
        params['xFilterReso'] = clamp(0.05 + h * 0.15, 0.0, 1.0)
    else:
        params['xFilterReso'] = clamp(0.2 + h * 0.4, 0.0, 1.0)

    # Detune
    if any(t in tags for t in ['detuned', 'thick', 'fat', 'wide']):
        params['xDetune'] = clamp(15 + h * 30, 0, 50)
    elif any(t in tags for t in ['pure', 'clean', 'precise']):
        params['xDetune'] = clamp(h * 5, 0, 50)
    else:
        params['xDetune'] = clamp(5 + h * 20, 0, 50)

    # Level based on balance
    if balance == 'X-dominant':
        params['xLevel'] = clamp(0.75 + h * 0.25, 0.0, 1.0)
    elif balance == 'O-dominant':
        params['xLevel'] = clamp(0.2 + h * 0.3, 0.0, 1.0)
    else:
        params['xLevel'] = clamp(0.6 + h * 0.3, 0.0, 1.0)

    # Pitch lock
    params['xPitchLock'] = 1 if any(t in tags for t in ['pitched', 'melodic', 'tonal']) else 0

    # Unison
    if any(t in tags for t in ['thick', 'fat', 'wide', 'massive']):
        params['xUnison'] = 2  # 4 voices
    elif any(t in tags for t in ['stereo', 'spread']):
        params['xUnison'] = 1  # 2 voices
    else:
        params['xUnison'] = 0 if h < 0.5 else 1

    # Polyphony
    if category in ('Floating', 'Deep Space'):
        params['xPolyphony'] = 2 if h < 0.5 else 3  # 4 or 8
    elif category == 'Grounded':
        params['xPolyphony'] = 0 if h < 0.3 else (1 if h < 0.7 else 2)
    else:
        params['xPolyphony'] = int(h * 3.99)

    # ---- Engine O parameters ----
    # Morph (0=sine, 1=triangle, 2=saw, 3=square-ish)
    if any(t in tags for t in ['warm', 'sine', 'pure', 'soft']):
        params['oMorph'] = clamp(0.0 + h * 0.8, 0.0, 3.0)
    elif any(t in tags for t in ['bright', 'saw', 'harsh', 'cutting']):
        params['oMorph'] = clamp(1.5 + h * 1.0, 0.0, 3.0)
    elif any(t in tags for t in ['square', 'hollow', 'pulse']):
        params['oMorph'] = clamp(2.2 + h * 0.8, 0.0, 3.0)
    elif category == 'Deep Space':
        params['oMorph'] = clamp(0.5 + h * 2.0, 0.0, 3.0)
    elif category == 'Floating':
        params['oMorph'] = clamp(0.2 + h * 1.5, 0.0, 3.0)
    else:
        params['oMorph'] = clamp(h * 3.0, 0.0, 3.0)

    # Bloom (attack)
    if any(t in tags for t in ['slow', 'ambient', 'pad', 'evolving']):
        params['oBloom'] = clamp(2.0 + h * 6.0, 0.001, 10.0)
    elif any(t in tags for t in ['fast', 'percussive', 'sharp', 'snap']):
        params['oBloom'] = clamp(0.01 + h * 0.3, 0.001, 10.0)
    elif category == 'Deep Space':
        params['oBloom'] = clamp(1.5 + h * 5.0, 0.001, 10.0)
    elif category == 'Floating':
        params['oBloom'] = clamp(1.0 + h * 4.0, 0.001, 10.0)
    elif category == 'Grounded':
        params['oBloom'] = clamp(0.1 + h * 1.5, 0.001, 10.0)
    else:
        params['oBloom'] = clamp(0.5 + h * 3.0, 0.001, 10.0)

    # Decay
    if category == 'Deep Space':
        params['oDecay'] = clamp(3.0 + h * 5.0, 0.01, 8.0)
    elif category == 'Floating':
        params['oDecay'] = clamp(2.0 + h * 4.0, 0.01, 8.0)
    elif category == 'Grounded':
        params['oDecay'] = clamp(0.5 + h * 2.5, 0.01, 8.0)
    else:
        params['oDecay'] = clamp(1.0 + h * 4.0, 0.01, 8.0)

    # Sustain
    if category in ('Floating', 'Deep Space'):
        params['oSustain'] = clamp(0.5 + h * 0.45, 0.0, 1.0)
    elif category == 'Grounded':
        params['oSustain'] = clamp(0.1 + h * 0.5, 0.0, 1.0)
    else:
        params['oSustain'] = clamp(0.3 + h * 0.6, 0.0, 1.0)

    # Release
    if category == 'Deep Space':
        params['oRelease'] = clamp(2.0 + h * 7.0, 0.01, 10.0)
    elif category == 'Floating':
        params['oRelease'] = clamp(1.5 + h * 5.0, 0.01, 10.0)
    elif category == 'Grounded':
        params['oRelease'] = clamp(0.1 + h * 1.5, 0.01, 10.0)
    else:
        params['oRelease'] = clamp(0.5 + h * 3.0, 0.01, 10.0)

    # O Filter
    if any(t in tags for t in ['dark', 'deep', 'muffled', 'sub']):
        params['oFilterCutoff'] = clamp(150 + h * 1200, 20, 20000)
    elif any(t in tags for t in ['bright', 'open', 'airy']):
        params['oFilterCutoff'] = clamp(3000 + h * 10000, 20, 20000)
    elif category == 'Deep Space':
        params['oFilterCutoff'] = clamp(600 + h * 4000, 20, 20000)
    elif category == 'Floating':
        params['oFilterCutoff'] = clamp(800 + h * 5000, 20, 20000)
    else:
        params['oFilterCutoff'] = clamp(800 + h * 6000, 20, 20000)

    if any(t in tags for t in ['resonant', 'squelchy']):
        params['oFilterReso'] = clamp(0.6 + h * 0.35, 0.0, 1.0)
    else:
        params['oFilterReso'] = clamp(0.2 + h2 * 0.5, 0.0, 1.0)

    # Drift
    if any(t in tags for t in ['analog', 'unstable', 'warble', 'drift']):
        params['oDrift'] = clamp(0.4 + h * 0.5, 0.0, 1.0)
    elif any(t in tags for t in ['digital', 'precise', 'clean']):
        params['oDrift'] = clamp(0.0 + h * 0.1, 0.0, 1.0)
    elif category == 'Deep Space':
        params['oDrift'] = clamp(0.3 + h * 0.5, 0.0, 1.0)
    else:
        params['oDrift'] = clamp(0.1 + h * 0.4, 0.0, 1.0)

    # Sub
    if any(t in tags for t in ['sub', 'bass', 'deep', 'heavy']):
        params['oSub'] = clamp(0.5 + h * 0.5, 0.0, 1.0)
    elif category == 'Floating':
        params['oSub'] = clamp(0.1 + h * 0.3, 0.0, 1.0)
    elif category == 'Deep Space':
        params['oSub'] = clamp(0.3 + h * 0.5, 0.0, 1.0)
    else:
        params['oSub'] = clamp(0.2 + h * 0.5, 0.0, 1.0)

    # O Detune
    if any(t in tags for t in ['unison', 'thick', 'detuned', 'beating']):
        params['oDetune'] = clamp(15 + h * 30, 0, 50)
    elif any(t in tags for t in ['pure', 'clean']):
        params['oDetune'] = clamp(h * 5, 0, 50)
    else:
        params['oDetune'] = clamp(5 + h * 20, 0, 50)

    # O Level
    if balance == 'O-dominant':
        params['oLevel'] = clamp(0.75 + h * 0.25, 0.0, 1.0)
    elif balance == 'X-dominant':
        params['oLevel'] = clamp(0.2 + h * 0.3, 0.0, 1.0)
    else:
        params['oLevel'] = clamp(0.6 + h * 0.3, 0.0, 1.0)

    # O Polyphony
    if category in ('Floating', 'Deep Space'):
        params['oPolyphony'] = 3 if h < 0.5 else 4  # 8 or 16
    elif category == 'Grounded':
        params['oPolyphony'] = 1 if h < 0.5 else 2  # 2 or 4
    else:
        params['oPolyphony'] = int(1 + h * 3.99)

    # ---- Macros ----
    params['macro1'] = clamp(0.4 + (h - 0.5) * 0.3, 0.0, 1.0)
    params['macro2'] = clamp(0.5 + (h2 - 0.5) * 0.4, 0.0, 1.0)
    params['macro3'] = clamp(0.5 + (h3 - 0.5) * 0.3, 0.0, 1.0)
    params['macro4'] = clamp(0.5 + (h - 0.5) * 0.2, 0.0, 1.0)

    # ---- Effects ----

    # Delay
    params['delayEnabled'] = 1
    if tempo and tempo > 0:
        # Sync delay to tempo: use dotted eighth or quarter
        beat_sec = 60.0 / tempo
        delay_options = [beat_sec * 0.75, beat_sec * 0.5, beat_sec, beat_sec * 1.5]
        params['delayTime'] = clamp(delay_options[int(h * len(delay_options))], 0.01, 2.0)
    else:
        params['delayTime'] = clamp(0.2 + h * 0.8, 0.01, 2.0)

    if any(t in tags for t in ['dub', 'echo', 'delay', 'spacious']):
        params['delayFeedback'] = clamp(0.55 + h * 0.35, 0.0, 0.95)
        params['delayMix'] = clamp(0.3 + h * 0.35, 0.0, 1.0)
    elif category == 'Deep Space':
        params['delayFeedback'] = clamp(0.5 + h * 0.4, 0.0, 0.95)
        params['delayMix'] = clamp(0.25 + h * 0.3, 0.0, 1.0)
    elif category == 'Grounded':
        params['delayFeedback'] = clamp(0.2 + h * 0.3, 0.0, 0.95)
        params['delayMix'] = clamp(0.1 + h * 0.25, 0.0, 1.0)
    else:
        params['delayFeedback'] = clamp(0.3 + h * 0.4, 0.0, 0.95)
        params['delayMix'] = clamp(0.2 + h * 0.3, 0.0, 1.0)

    if any(t in tags for t in ['tape', 'lo-fi', 'warble', 'cassette', 'wobble']):
        params['delayWobble'] = clamp(0.3 + h * 0.5, 0.0, 1.0)
    elif category == 'Deep Space':
        params['delayWobble'] = clamp(0.1 + h * 0.3, 0.0, 1.0)
    else:
        params['delayWobble'] = clamp(0.05 + h * 0.25, 0.0, 1.0)

    # Reverb
    params['reverbEnabled'] = 1
    if category == 'Deep Space':
        params['reverbDecay'] = clamp(0.8 + h * 0.18, 0.1, 0.99)
        params['reverbSize'] = clamp(1.3 + h * 0.7, 0.1, 2.0)
        params['reverbMix'] = clamp(0.35 + h * 0.35, 0.0, 1.0)
    elif category == 'Floating':
        params['reverbDecay'] = clamp(0.6 + h * 0.3, 0.1, 0.99)
        params['reverbSize'] = clamp(0.8 + h * 0.8, 0.1, 2.0)
        params['reverbMix'] = clamp(0.25 + h * 0.35, 0.0, 1.0)
    elif category == 'Grounded':
        params['reverbDecay'] = clamp(0.3 + h * 0.35, 0.1, 0.99)
        params['reverbSize'] = clamp(0.3 + h * 0.6, 0.1, 2.0)
        params['reverbMix'] = clamp(0.1 + h * 0.2, 0.0, 1.0)
    else:
        params['reverbDecay'] = clamp(0.5 + h * 0.35, 0.1, 0.99)
        params['reverbSize'] = clamp(0.6 + h * 0.8, 0.1, 2.0)
        params['reverbMix'] = clamp(0.2 + h * 0.3, 0.0, 1.0)

    if any(t in tags for t in ['dark', 'muffled', 'warm']):
        params['reverbDamping'] = clamp(0.5 + h * 0.4, 0.0, 1.0)
    elif any(t in tags for t in ['bright', 'airy', 'open']):
        params['reverbDamping'] = clamp(0.05 + h * 0.2, 0.0, 1.0)
    else:
        params['reverbDamping'] = clamp(0.2 + h * 0.4, 0.0, 1.0)

    # Phaser
    if any(t in tags for t in ['phaser', 'phasing', 'sweep', 'swirl']):
        params['phaserEnabled'] = 1
        params['phaserRate'] = clamp(0.1 + h * 2.0, 0.01, 10.0)
        params['phaserDepth'] = clamp(0.4 + h * 0.5, 0.0, 1.0)
        params['phaserMix'] = clamp(0.3 + h * 0.4, 0.0, 1.0)
    elif category == 'Deep Space' and h > 0.4:
        params['phaserEnabled'] = 1
        params['phaserRate'] = clamp(0.05 + h * 0.5, 0.01, 10.0)
        params['phaserDepth'] = clamp(0.3 + h * 0.4, 0.0, 1.0)
        params['phaserMix'] = clamp(0.2 + h * 0.3, 0.0, 1.0)
    else:
        params['phaserEnabled'] = 0
        params['phaserRate'] = 0.5
        params['phaserDepth'] = 0.5
        params['phaserMix'] = 0.3

    # Lo-Fi
    if any(t in tags for t in ['lo-fi', 'lofi', 'tape', 'cassette', 'degradation', 'dusty', 'grainy']):
        params['lofiEnabled'] = 1
        params['lofiBitDepth'] = clamp(6 + h * 6, 2, 16)
        params['lofiRateReduce'] = clamp(1 + h * 15, 1, 64)
        params['lofiWowFlutter'] = clamp(0.2 + h * 0.5, 0.0, 1.0)
        params['lofiMix'] = clamp(0.3 + h * 0.4, 0.0, 1.0)
    elif any(t in tags for t in ['glitch', 'digital', 'broken']):
        params['lofiEnabled'] = 1
        params['lofiBitDepth'] = clamp(4 + h * 4, 2, 16)
        params['lofiRateReduce'] = clamp(2 + h * 20, 1, 64)
        params['lofiWowFlutter'] = clamp(h * 0.3, 0.0, 1.0)
        params['lofiMix'] = clamp(0.25 + h * 0.35, 0.0, 1.0)
    else:
        params['lofiEnabled'] = 0
        params['lofiBitDepth'] = 12.0
        params['lofiRateReduce'] = 1.0
        params['lofiWowFlutter'] = 0.0
        params['lofiMix'] = 0.3

    # Compressor
    params['compressorEnabled'] = 1
    if any(t in tags for t in ['heavy', 'pumping', 'compressed', 'sidechain']):
        params['compThreshold'] = clamp(-25 - h * 10, -40, 0)
        params['compRatio'] = clamp(6 + h * 10, 1, 20)
        params['compAttack'] = clamp(0.5 + h * 10, 0.1, 100)
        params['compRelease'] = clamp(50 + h * 200, 1, 1000)
    elif category == 'Deep Space':
        params['compThreshold'] = clamp(-8 - h * 8, -40, 0)
        params['compRatio'] = clamp(2 + h * 3, 1, 20)
        params['compAttack'] = clamp(10 + h * 30, 0.1, 100)
        params['compRelease'] = clamp(100 + h * 300, 1, 1000)
    else:
        params['compThreshold'] = clamp(-10 - h * 10, -40, 0)
        params['compRatio'] = clamp(2 + h * 5, 1, 20)
        params['compAttack'] = clamp(5 + h * 20, 0.1, 100)
        params['compRelease'] = clamp(80 + h * 150, 1, 1000)

    # Sequencer
    if has_seq and tempo:
        params['seqPlaying'] = 0  # Start paused, user presses play
        params['seqTempo'] = float(tempo)
    else:
        params['seqPlaying'] = 0
        params['seqTempo'] = float(tempo) if tempo else 120.0

    # Swing
    if any(t in tags for t in ['swing', 'shuffle', 'groove']):
        params['seqSwing'] = clamp(0.3 + h * 0.4, 0.0, 1.0)
    elif any(t in tags for t in ['dub', 'reggae']):
        params['seqSwing'] = clamp(0.15 + h * 0.2, 0.0, 1.0)
    else:
        params['seqSwing'] = clamp(h * 0.2, 0.0, 1.0)

    # Round floats for readability
    for k, v in params.items():
        if isinstance(v, float):
            params[k] = round(v, 3)

    # Generate sequencer pattern
    seq_data = None
    if has_seq:
        # Determine pattern style from tags/category
        if any(t in tags for t in ['four-on-the-floor', 'techno']):
            style = 'techno'
        elif any(t in tags for t in ['dnb', 'jungle', 'rolling']):
            style = 'dnb'
        elif any(t in tags for t in ['breakbeat', 'break', 'amen']):
            style = 'breakbeat'
        elif any(t in tags for t in ['dub', 'reggae', 'halftime', 'half-time']):
            style = 'dub'
        elif any(t in tags for t in ['sparse', 'minimal']):
            style = 'sparse'
        elif any(t in tags for t in ['triplet', 'tribal']):
            style = 'triplet'
        else:
            style = 'basic'

        num_steps = 16
        if any(t in tags for t in ['long', 'evolving']):
            num_steps = 32
        elif any(t in tags for t in ['short', 'minimal']):
            num_steps = 8

        seq_data = generate_sequencer_pattern(name, tempo or 120, num_steps, style)

    return params, seq_data


def process_preset_file(filepath):
    """Read a .xocmeta file, generate parameters, and write back."""
    with open(filepath, 'r') as f:
        meta = json.load(f)

    params, seq_data = generate_params(meta)

    meta['parameters'] = params
    if seq_data:
        meta['sequencer'] = seq_data
    elif 'sequencer' in meta:
        del meta['sequencer']

    with open(filepath, 'w') as f:
        json.dump(meta, f, indent=2)
        f.write('\n')

    return meta['name']


def main():
    count = 0
    for category in ['Grounded', 'Floating', 'Deep Space', 'Entangled']:
        cat_dir = os.path.join(PRESETS_DIR, category)
        if not os.path.isdir(cat_dir):
            continue
        cat_count = 0
        for filename in sorted(os.listdir(cat_dir)):
            if filename.endswith('.xocmeta'):
                filepath = os.path.join(cat_dir, filename)
                name = process_preset_file(filepath)
                cat_count += 1
                count += 1
        print(f"  {category}: {cat_count} presets updated")

    print(f"\nTotal: {count} presets updated with DSP parameters")


if __name__ == '__main__':
    main()
