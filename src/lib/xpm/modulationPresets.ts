import type { XpmModulationConfig } from './xpmTypes';

export interface ModulationPreset {
  id: string;
  name: string;
  icon: string;
  description: string;
  config: XpmModulationConfig;
}

export const MODULATION_PRESETS: ModulationPreset[] = [
  {
    id: 'velocity-dynamics',
    name: 'Velocity Dynamics',
    icon: '🎯',
    description: 'Classic velocity-sensitive filter opening',
    config: {
      routes: [
        { source: 'Velocity', destination: 'FilterCutoff', amount: 0.4 },
        { source: 'Velocity', destination: 'Volume', amount: 0.3 },
      ],
    },
  },
  {
    id: 'expressive-keys',
    name: 'Expressive Keys',
    icon: '🎹',
    description: 'Expressive keyboard response',
    config: {
      routes: [
        { source: 'Velocity', destination: 'FilterCutoff', amount: 0.6 },
        { source: 'Velocity', destination: 'Pitch', amount: 0.05 },
        { source: 'Aftertouch', destination: 'FilterCutoff', amount: 0.3 },
      ],
    },
  },
  {
    id: 'living-pad',
    name: 'Living Pad',
    icon: '🌊',
    description: 'Gentle movement for sustained sounds',
    config: {
      routes: [
        { source: 'LFO1', destination: 'FilterCutoff', amount: 0.2 },
        { source: 'LFO1', destination: 'Pan', amount: 0.15 },
      ],
      lfo1: {
        rate: 0.3,
        shape: 0, // sine
        sync: false,
      },
    },
  },
  {
    id: 'wobble-bass',
    name: 'Wobble Bass',
    icon: '🔊',
    description: 'Heavy LFO wobble with velocity dynamics',
    config: {
      routes: [
        { source: 'LFO1', destination: 'FilterCutoff', amount: 0.7 },
        { source: 'Velocity', destination: 'Volume', amount: 0.4 },
      ],
      lfo1: {
        rate: 0.6,
        shape: 1, // triangle
        sync: false,
      },
    },
  },
  {
    id: 'breathing-texture',
    name: 'Breathing Texture',
    icon: '🍃',
    description: 'Subtle organic breathing',
    config: {
      routes: [
        { source: 'LFO1', destination: 'Volume', amount: 0.15 },
        { source: 'LFO1', destination: 'FilterCutoff', amount: 0.1 },
        { source: 'Velocity', destination: 'FilterResonance', amount: 0.3 },
      ],
      lfo1: {
        rate: 0.15,
        shape: 0, // sine
        sync: false,
      },
    },
  },
  {
    id: 'aggressive-hit',
    name: 'Aggressive Hit',
    icon: '💥',
    description: 'Punchy velocity response for drums',
    config: {
      routes: [
        { source: 'Velocity', destination: 'FilterCutoff', amount: 0.8 },
        { source: 'Velocity', destination: 'Volume', amount: 0.5 },
        { source: 'Velocity', destination: 'Pitch', amount: 0.02 },
      ],
    },
  },
  {
    id: 'drift-machine',
    name: 'Drift Machine',
    icon: '🌀',
    description: 'Analog drift simulation',
    config: {
      routes: [
        { source: 'LFO1', destination: 'Pitch', amount: 0.08 },
        { source: 'LFO1', destination: 'FilterCutoff', amount: 0.12 },
        { source: 'LFO1', destination: 'Pan', amount: 0.2 },
      ],
      lfo1: {
        rate: 0.1,
        shape: 4, // random
        sync: false,
      },
    },
  },
  {
    id: 'filter-sweep',
    name: 'Filter Sweep',
    icon: '🎛️',
    description: 'Manual filter control via mod wheel',
    config: {
      routes: [
        { source: 'ModWheel', destination: 'FilterCutoff', amount: 0.9 },
        { source: 'ModWheel', destination: 'FilterResonance', amount: 0.3 },
      ],
    },
  },
];
