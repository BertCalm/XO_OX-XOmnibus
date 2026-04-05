import type { XpmProgramData, XpmInstrumentData, XpmLayerData, XpmModulationConfig } from './xpmTypes';
import { getDefaultVersion, fmt, fmtBool } from './xpmTemplate';

/**
 * Generate a complete XPM XML string from program data.
 * Produces valid MPC 2.x compatible XML.
 */
export function generateXpmXml(program: XpmProgramData): string {
  const version = getDefaultVersion();
  const lines: string[] = [];

  lines.push('<?xml version="1.0" encoding="UTF-8"?>');
  lines.push('<MPCVObject>');

  // Version block
  lines.push('  <Version>');
  lines.push(`    <File_Version>${version.fileVersion}</File_Version>`);
  lines.push(`    <Application>${version.application}</Application>`);
  lines.push(`    <Application_Version>${version.appVersion}</Application_Version>`);
  lines.push(`    <Platform>${version.platform}</Platform>`);
  lines.push('  </Version>');

  // Program block
  lines.push(`  <Program type="${program.type}">`);
  lines.push(`    <ProgramName>${escapeXml(program.name)}</ProgramName>`);
  lines.push(`    <AudioRoute>0</AudioRoute>`);
  lines.push(`    <Send1>${fmt(program.send1)}</Send1>`);
  lines.push(`    <Send2>${fmt(program.send2)}</Send2>`);
  lines.push(`    <Send3>${fmt(program.send3)}</Send3>`);
  lines.push(`    <Send4>${fmt(program.send4)}</Send4>`);
  lines.push(`    <Volume>${fmt(program.volume)}</Volume>`);
  lines.push(`    <Mute>${fmtBool(program.mute)}</Mute>`);
  lines.push(`    <Solo>${fmtBool(program.solo)}</Solo>`);
  lines.push(`    <Pan>${fmt(program.pan)}</Pan>`);
  lines.push(`    <Pitch>${fmt(program.pitch)}</Pitch>`);
  lines.push(`    <TuneCoarse>${program.tuneCoarse}</TuneCoarse>`);
  lines.push(`    <TuneFine>${program.tuneFine}</TuneFine>`);

  // Keygroup settings — must precede <Instruments> per MPC XPM schema
  if (program.type === 'Keygroup') {
    lines.push(`    <KeygroupNumKeygroups>${program.keygroupNumKeygroups}</KeygroupNumKeygroups>`);
    lines.push(`    <KeygroupPitchBendRange>${fmt(program.keygroupPitchBendRange)}</KeygroupPitchBendRange>`);
    lines.push(`    <KeygroupWheelToLfo>${fmt(program.keygroupWheelToLfo)}</KeygroupWheelToLfo>`);
  }

  // Instruments
  lines.push('    <Instruments>');
  for (const instrument of program.instruments) {
    renderInstrument(lines, instrument);
  }
  lines.push('    </Instruments>');

  // Pad Note Map
  if (program.padNoteMap.length > 0) {
    lines.push('    <PadNoteMap>');
    for (const mapping of program.padNoteMap) {
      lines.push(`      <PadNote number="${mapping.padNumber}">`);
      lines.push(`        <Note>${mapping.note}</Note>`);
      lines.push('      </PadNote>');
    }
    lines.push('    </PadNoteMap>');
  }

  // Pad Group Map
  if (program.padGroupMap.length > 0) {
    lines.push('    <PadGroupMap>');
    for (const mapping of program.padGroupMap) {
      lines.push(`      <PadGroup number="${mapping.padNumber}">`);
      lines.push(`        <Group>${mapping.group}</Group>`);
      lines.push('      </PadGroup>');
    }
    lines.push('    </PadGroupMap>');
  }

  lines.push('  </Program>');
  lines.push('</MPCVObject>');

  return lines.join('\n');
}

function renderInstrument(lines: string[], inst: XpmInstrumentData): void {
  lines.push(`      <Instrument number="${inst.number}">`);
  lines.push(`        <LowNote>${inst.lowNote}</LowNote>`);
  lines.push(`        <HighNote>${inst.highNote}</HighNote>`);
  lines.push(`        <IgnoreBaseNote>${fmtBool(inst.ignoreBaseNote)}</IgnoreBaseNote>`);
  lines.push(`        <ZonePlay>${inst.zonePlay}</ZonePlay>`);
  lines.push(`        <TriggerMode>${inst.triggerMode}</TriggerMode>`);

  // Filter
  lines.push(`        <FilterType>${inst.filterType}</FilterType>`);
  lines.push(`        <Cutoff>${fmt(inst.cutoff)}</Cutoff>`);
  lines.push(`        <Resonance>${fmt(inst.resonance)}</Resonance>`);
  lines.push(`        <FilterEnvAmt>${fmt(inst.filterEnvAmt)}</FilterEnvAmt>`);
  lines.push(`        <FilterAttack>${fmt(inst.filterAttack)}</FilterAttack>`);
  lines.push(`        <FilterDecay>${fmt(inst.filterDecay)}</FilterDecay>`);
  lines.push(`        <FilterSustain>${fmt(inst.filterSustain)}</FilterSustain>`);
  lines.push(`        <FilterRelease>${fmt(inst.filterRelease)}</FilterRelease>`);
  lines.push(`        <FilterHold>${fmt(inst.filterHold)}</FilterHold>`);

  // Volume Envelope
  lines.push(`        <VolumeAttack>${fmt(inst.volumeAttack)}</VolumeAttack>`);
  lines.push(`        <VolumeHold>${fmt(inst.volumeHold)}</VolumeHold>`);
  lines.push(`        <VolumeDecay>${fmt(inst.volumeDecay)}</VolumeDecay>`);
  lines.push(`        <VolumeSustain>${fmt(inst.volumeSustain)}</VolumeSustain>`);
  lines.push(`        <VolumeRelease>${fmt(inst.volumeRelease)}</VolumeRelease>`);
  lines.push(`        <VelocitySensitivity>${fmt(inst.velocitySensitivity)}</VelocitySensitivity>`);

  // Pitch Envelope
  lines.push(`        <PitchAttack>${fmt(inst.pitchAttack)}</PitchAttack>`);
  lines.push(`        <PitchHold>${fmt(inst.pitchHold)}</PitchHold>`);
  lines.push(`        <PitchDecay>${fmt(inst.pitchDecay)}</PitchDecay>`);
  lines.push(`        <PitchSustain>${fmt(inst.pitchSustain)}</PitchSustain>`);
  lines.push(`        <PitchRelease>${fmt(inst.pitchRelease)}</PitchRelease>`);
  lines.push(`        <PitchEnvAmount>${fmt(inst.pitchEnvAmount)}</PitchEnvAmount>`);

  // Layers
  lines.push('        <Layers>');
  for (const layer of inst.layers) {
    renderLayer(lines, layer);
  }
  lines.push('        </Layers>');

  // Modulation routing
  if (inst.modulation && inst.modulation.routes.length > 0) {
    renderModulation(lines, inst.modulation);
  }

  lines.push('      </Instrument>');
}

function renderLayer(lines: string[], layer: XpmLayerData): void {
  lines.push(`          <Layer number="${layer.number}">`);
  lines.push(`            <Active>${fmtBool(layer.active)}</Active>`);
  lines.push(`            <Volume>${fmt(layer.volume)}</Volume>`);
  lines.push(`            <Pan>${fmt(layer.pan)}</Pan>`);
  lines.push(`            <Pitch>${fmt(layer.pitch)}</Pitch>`);
  lines.push(`            <TuneCoarse>${layer.tuneCoarse}</TuneCoarse>`);
  lines.push(`            <TuneFine>${layer.tuneFine}</TuneFine>`);
  lines.push(`            <VelStart>${layer.velStart}</VelStart>`);
  lines.push(`            <VelEnd>${layer.velEnd}</VelEnd>`);
  lines.push(`            <SampleName>${escapeXml(layer.sampleName)}</SampleName>`);
  lines.push(`            <SampleFile>${escapeXml(layer.sampleFile)}</SampleFile>`);
  lines.push(`            <RootNote>${layer.rootNote}</RootNote>`);
  lines.push(`            <KeyTrack>${fmtBool(layer.keyTrack)}</KeyTrack>`);
  lines.push(`            <SliceStart>${layer.sliceStart}</SliceStart>`);
  lines.push(`            <SliceEnd>${layer.sliceEnd}</SliceEnd>`);
  lines.push(`            <SliceLoop>${layer.sliceLoop}</SliceLoop>`);
  lines.push(`            <SliceLoopStart>${layer.sliceLoopStart}</SliceLoopStart>`);
  lines.push(`            <SliceLoopCrossFadeLength>${layer.sliceLoopCrossFadeLength}</SliceLoopCrossFadeLength>`);
  lines.push(`            <PitchRandom>${fmt(layer.pitchRandom)}</PitchRandom>`);
  lines.push(`            <VolumeRandom>${fmt(layer.volumeRandom)}</VolumeRandom>`);
  lines.push(`            <PanRandom>${fmt(layer.panRandom)}</PanRandom>`);
  lines.push(`            <SliceIndex>${layer.sliceIndex}</SliceIndex>`);
  lines.push(`            <Direction>${layer.direction}</Direction>`);
  lines.push(`            <Offset>${layer.offset}</Offset>`);
  // Probability tag — always emit for MPC firmware compatibility (default 100)
  lines.push(`            <Probability>${layer.probability ?? 100}</Probability>`);
  lines.push('          </Layer>');
}

function renderModulation(lines: string[], mod: XpmModulationConfig): void {
  lines.push('        <Modulation>');

  // LFO1 settings (emitted first if present)
  if (mod.lfo1) {
    lines.push('          <LFO1>');
    lines.push(`            <Rate>${fmt(mod.lfo1.rate)}</Rate>`);
    lines.push(`            <Shape>${mod.lfo1.shape}</Shape>`);
    lines.push(`            <Sync>${fmtBool(mod.lfo1.sync)}</Sync>`);
    lines.push('          </LFO1>');
  }

  // Modulation routes
  for (let i = 0; i < mod.routes.length; i++) {
    const route = mod.routes[i];
    lines.push(`          <Route number="${i + 1}">`);
    lines.push(`            <Source>${route.source}</Source>`);
    lines.push(`            <Destination>${route.destination}</Destination>`);
    lines.push(`            <Amount>${fmt(route.amount)}</Amount>`);
    lines.push('          </Route>');
  }

  lines.push('        </Modulation>');
}

function escapeXml(str: string): string {
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&apos;');
}
