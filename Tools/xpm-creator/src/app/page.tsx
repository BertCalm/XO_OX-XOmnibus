'use client';

import React, { useState, useCallback, useMemo, useRef } from 'react';
import dynamic from 'next/dynamic';
import Header from '@/components/layout/Header';
import Sidebar from '@/components/layout/Sidebar';
import WorkspaceLayout from '@/components/layout/WorkspaceLayout';
import AudioImporter from '@/components/audio/AudioImporter';
import SampleList from '@/components/audio/SampleList';
import BatchTools from '@/components/audio/BatchTools';
import SampleRenamer from '@/components/audio/SampleRenamer';
import SamplePlayer from '@/components/audio/SamplePlayer';
import PadGrid from '@/components/pads/PadGrid';
import DrumKitBuilder from '@/components/pads/DrumKitBuilder';
import Card, { CardHeader, CardTitle } from '@/components/ui/Card';
import Tabs from '@/components/ui/Tabs';
import Modal from '@/components/ui/Modal';
import Button from '@/components/ui/Button';
import ErrorBoundary from '@/components/ui/ErrorBoundary';
import type { CommandAction } from '@/components/ui/CommandPalette';
import ThemeProvider from '@/components/layout/ThemeProvider';
import TextureOverlay from '@/components/ui/TextureOverlay';
import QuickStart from '@/components/ui/QuickStart';
import ToastContainer from '@/components/ui/ToastContainer';
import StatusBar from '@/components/layout/StatusBar';
import { useAudioStore } from '@/stores/audioStore';
import { useProjectStore } from '@/stores/projectStore';
import { useErrorStore } from '@/stores/errorStore';
import { useLayoutStore } from '@/stores/layoutStore';
import { useHotkeys } from '@/hooks/useHotkeys';
import { useAutoRestore } from '@/hooks/useAutoRestore';
import { useAutoSave } from '@/hooks/useAutoSave';
import { useMilestoneTracker } from '@/hooks/useMilestoneTracker';
import { PROJECT_TEMPLATES } from '@/constants/projectTemplates';
import type { ProgramType } from '@/types';
import { validateName } from '@/lib/sanitize';

// ── Dynamic imports for components not needed at initial render ─────────────
// These split the JS bundle so users get a fast first load, then heavy panels
// load on-demand when they navigate to them or open modals.

const WaveformEditor = dynamic(() => import('@/components/audio/WaveformEditor'), {
  ssr: false,
  loading: () => <div className="h-32 animate-pulse bg-surface-alt rounded-lg" />,
});

const PadLayerEditor = dynamic(() => import('@/components/pads/PadLayerEditor'), {
  ssr: false,
  loading: () => <div className="h-64 animate-pulse bg-surface-alt rounded-lg" />,
});

const LayerAssignment = dynamic(() => import('@/components/pads/LayerAssignment'), {
  ssr: false,
});

const ProgramEditor = dynamic(() => import('@/components/program/ProgramEditor'), {
  ssr: false,
});

const HumanizeControls = dynamic(() => import('@/components/program/HumanizeControls'), {
  ssr: false,
});

const MuteGroupGrid = dynamic(() => import('@/components/pads/MuteGroupGrid'), {
  ssr: false,
});

const KitImporter = dynamic(() => import('@/components/import/KitImporter'), {
  ssr: false,
});

const XpmExporter = dynamic(() => import('@/components/export/XpmExporter'), {
  ssr: false,
});

const XpnPackager = dynamic(() => import('@/components/export/XpnPackager'), {
  ssr: false,
});

const RamEstimator = dynamic(() => import('@/components/export/RamEstimator'), {
  ssr: false,
});

const KeyboardShortcuts = dynamic(() => import('@/components/ui/KeyboardShortcuts'), {
  ssr: false,
});

const CommandPalette = dynamic(() => import('@/components/ui/CommandPalette'), {
  ssr: false,
});

const ErrorLogViewer = dynamic(() => import('@/components/ui/ErrorLogViewer'), {
  ssr: false,
});

const CloudBrowser = dynamic(() => import('@/components/cloud/CloudBrowser'), {
  ssr: false,
});

const CloudExportPanel = dynamic(() => import('@/components/cloud/CloudExportPanel'), {
  ssr: false,
});

const MidiPanel = dynamic(() => import('@/components/midi/MidiPanel'), {
  ssr: false,
});

const ABCompare = dynamic(() => import('@/components/audio/ABCompare'), {
  ssr: false,
});

type SidebarSection = 'samples' | 'pads' | 'program' | 'export';

export default function HomePage() {
  const [section, setSection] = useState<SidebarSection>('samples');
  const [showNewProject, setShowNewProject] = useState(false);
  const [showShortcuts, setShowShortcuts] = useState(false);
  const [exportMode, setExportMode] = useState<'xpm' | 'xpn'>('xpm');
  const [projectName, setProjectName] = useState('');
  const [programType, setProgramType] = useState<ProgramType>('Keygroup');
  const [selectedTemplateId, setSelectedTemplateId] = useState<string | null>(null);

  const audioImporterRef = useRef<HTMLDivElement>(null);
  const kitImporterRef = useRef<HTMLDivElement>(null);

  // Granular selectors — avoid subscribing to entire stores
  const samples = useAudioStore((s) => s.samples);
  const activeSampleId = useAudioStore((s) => s.activeSampleId);
  const currentProject = useProjectStore((s) => s.currentProject);
  const createProject = useProjectStore((s) => s.createProject);

  const glassEnabled = useLayoutStore((s) => s.glassEnabled);

  const [showCommandPalette, setShowCommandPalette] = useState(false);
  const [showErrorLog, setShowErrorLog] = useState(false);
  const [showCloudBrowser, setShowCloudBrowser] = useState(false);
  const errorCount = useErrorStore((s) => s.errors.filter((e) => !e.resolved).length);

  const toggleShortcuts = useCallback(() => {
    setShowShortcuts((prev) => !prev);
  }, []);

  const toggleCommandPalette = useCallback(() => {
    setShowCommandPalette((prev) => !prev);
  }, []);

  // Global keyboard shortcuts
  useHotkeys({
    activeSection: section,
    onSectionChange: setSection,
    onToggleShortcuts: toggleShortcuts,
    onToggleCommandPalette: toggleCommandPalette,
  });

  // Restore last project from IndexedDB on mount (before auto-save kicks in)
  useAutoRestore();

  // Auto-save project state to IndexedDB
  useAutoSave();

  // Achievement milestone tracking
  useMilestoneTracker();

  // Command palette actions
  const commands: CommandAction[] = useMemo(
    () => [
      // Navigation
      { id: 'nav-crate', label: 'Go to The Crate', description: 'Sample import & editing', icon: '\uD83D\uDCE6', category: 'Navigation', shortcut: '1', action: () => setSection('samples') },
      { id: 'nav-forge', label: 'Go to The Forge', description: 'Pad configuration', icon: '\u2692\uFE0F', category: 'Navigation', shortcut: '2', action: () => setSection('pads') },
      { id: 'nav-program', label: 'Go to Program Editor', icon: '\uD83C\uDF9B\uFE0F', category: 'Navigation', shortcut: '3', action: () => setSection('program') },
      { id: 'nav-export', label: 'Go to The Breath', description: 'Export XPM/XPN', icon: '\uD83D\uDCE4', category: 'Navigation', shortcut: '4', action: () => setSection('export') },

      // Pad Tools
      { id: 'tool-vibe', label: 'Vibe Check', description: 'Randomize for boutique character', icon: '\u2728', category: 'Pad Tools', action: () => setSection('pads') },
      { id: 'tool-cycle', label: '8-Layer Cycle Engine', description: '4 vel tiers \u00D7 2 round robin', icon: '\uD83D\uDD04', category: 'Pad Tools', action: () => setSection('pads') },
      { id: 'tool-air', label: 'Spectral Air Injector', description: 'Add shimmer to high-velocity hits', icon: '\uD83C\uDF2C\uFE0F', category: 'Pad Tools', action: () => setSection('pads') },
      { id: 'tool-space', label: 'Space Fold Stereo', description: 'Velocity-driven stereo width', icon: '\uD83C\uDF0C', category: 'Pad Tools', action: () => setSection('pads') },
      { id: 'tool-choke', label: 'Auto-Choke Intelligence', description: 'Smart mute group linking', icon: '\uD83D\uDD17', category: 'Pad Tools', action: () => setSection('pads') },

      // Audio Processing
      { id: 'proc-tail', label: 'Batch Tail Taming', description: 'Remove clicks from all samples', icon: '\uD83D\uDD07', category: 'Audio', action: () => setSection('pads') },
      { id: 'proc-time', label: 'Time Traveler', description: 'Era-specific vintage processing', icon: '\u231B', category: 'Audio', action: () => setSection('pads') },
      { id: 'proc-kit', label: 'Drum Kit Builder', description: 'Auto-assign samples to pad layout', icon: '\uD83E\uDD41', category: 'Audio', action: () => setSection('pads') },

      // Project
      { id: 'proj-new', label: 'New Project', icon: '\uD83D\uDCC1', category: 'Project', action: () => setShowNewProject(true) },
      { id: 'proj-export', label: 'Export', icon: '\uD83D\uDCE4', category: 'Project', action: () => setSection('export') },

      // Settings
      { id: 'set-theme', label: 'Change Theme', description: 'Switch between 20 themes', icon: '\uD83C\uDFA8', category: 'Settings', action: () => {} },
      { id: 'set-keys', label: 'Keyboard Shortcuts', description: 'View all shortcuts', icon: '\u2328\uFE0F', category: 'Settings', shortcut: '?', action: () => toggleShortcuts() },
      { id: 'set-errors', label: 'Error Log', description: 'View and manage error reports', icon: '\uD83D\uDCCB', category: 'Settings', action: () => setShowErrorLog(true) },
      { id: 'set-cloud', label: 'Cloud Storage', description: 'Import/export to Google Drive, OneDrive, Dropbox', icon: '\u2601\uFE0F', category: 'Settings', action: () => setShowCloudBrowser(true) },
    ],
    [toggleShortcuts]
  );

  const activeSample = useMemo(
    () => samples.find((s) => s.id === activeSampleId) ?? null,
    [samples, activeSampleId]
  );

  const handleCreateProject = async () => {
    if (!projectName.trim()) return;
    await createProject(validateName(projectName), programType, selectedTemplateId ?? undefined);
    setShowNewProject(false);
    setProjectName('');
    setSelectedTemplateId(null);
  };

  const scrollToImporter = useCallback(() => {
    audioImporterRef.current?.scrollIntoView({ behavior: 'smooth', block: 'center' });
  }, []);

  const scrollToKitImporter = useCallback(() => {
    kitImporterRef.current?.scrollIntoView({ behavior: 'smooth', block: 'center' });
  }, []);

  const renderSamplesSection = () => {
    if (samples.length === 0) {
      return (
        <ErrorBoundary section="The Crate">
          <div className="p-4 space-y-4">
            <QuickStart
              onImportSamples={scrollToImporter}
              onImportKit={scrollToKitImporter}
              onNewProject={() => setShowNewProject(true)}
            />
            <div ref={audioImporterRef}>
              <AudioImporter />
            </div>
            <div ref={kitImporterRef}>
              <KitImporter />
            </div>
          </div>
        </ErrorBoundary>
      );
    }

    return (
      <ErrorBoundary section="The Crate">
        <div className="p-4 space-y-4">
          <AudioImporter />

          {activeSample && (
            <Card>
              <CardHeader>
                <CardTitle>{activeSample.name}</CardTitle>
                <SamplePlayer sampleId={activeSample.id} buffer={activeSample.buffer} name={activeSample.name} />
              </CardHeader>
              <WaveformEditor sample={activeSample} />
            </Card>
          )}

          <KitImporter />
        </div>
      </ErrorBoundary>
    );
  };

  const renderPadsSection = () => (
    <ErrorBoundary section="The Forge">
      <div className="p-4 space-y-4">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
          <Card padding="sm">
            <div className="p-2 flex items-center justify-between">
              <CardTitle>Pad Grid</CardTitle>
              <DrumKitBuilder />
            </div>
            <div className="p-2">
              <PadGrid />
            </div>
          </Card>
          <Card padding="none">
            <PadLayerEditor />
          </Card>
        </div>
        <Card>
          <CardHeader>
            <CardTitle>Quick Assign</CardTitle>
          </CardHeader>
          <LayerAssignment />
        </Card>
      </div>
    </ErrorBoundary>
  );

  const renderProgramSection = () => (
    <ErrorBoundary section="Program Editor">
      <div className="p-4 space-y-4">
        <ProgramEditor programType={currentProject?.programType || 'Keygroup'} />
      </div>
    </ErrorBoundary>
  );

  const renderExportSection = () => (
    <ErrorBoundary section="The Breath">
      <div className="p-4 space-y-4">
        <Tabs
          tabs={[
            { id: 'xpm', label: 'XPM Program' },
            { id: 'xpn', label: 'XPN Expansion' },
          ]}
          activeTab={exportMode}
          onChange={(t) => setExportMode(t as 'xpm' | 'xpn')}
        />
        {exportMode === 'xpm' ? <XpmExporter /> : <XpnPackager />}
        <RamEstimator />
      </div>
    </ErrorBoundary>
  );

  const renderMainContent = () => {
    // Wrap in keyed div so React remounts on section change, triggering entrance animation
    const content = (() => {
      switch (section) {
        case 'samples':
          return renderSamplesSection();
        case 'pads':
          return renderPadsSection();
        case 'program':
          return renderProgramSection();
        case 'export':
          return renderExportSection();
      }
    })();
    return (
      <div key={section} className="animate-section-enter">
        {content}
      </div>
    );
  };

  const renderPanel = () => {
    if (section === 'samples') {
      return (
        <div className="p-3 space-y-3">
          <h3 className="text-xs font-semibold text-text-secondary uppercase tracking-wider px-1">
            Sample Library
          </h3>
          <BatchTools />
          <SampleRenamer />
          <SampleList />
          <ABCompare />
        </div>
      );
    }
    if (section === 'pads') {
      return (
        <div className="p-3 space-y-4">
          <MidiPanel />
          <Card padding="sm">
            <div className="p-2">
              <CardTitle>Mute Groups</CardTitle>
            </div>
            <div className="p-2">
              <MuteGroupGrid />
            </div>
          </Card>
          <HumanizeControls />
        </div>
      );
    }
    return null;
  };

  return (
    <div className="h-screen flex flex-col" data-glass-disabled={!glassEnabled ? 'true' : undefined}>
      <ThemeProvider />
      <TextureOverlay />
      <Header
        projectName={currentProject?.name || 'Untitled'}
        onNewProject={() => setShowNewProject(true)}
        onExport={() => setSection('export')}
        activeSection={
          section === 'samples'
            ? 'crate'
            : section === 'pads' || section === 'program'
              ? 'forge'
              : section === 'export'
                ? 'breath'
                : undefined
        }
      />

      <WorkspaceLayout
        sidebar={
          <Sidebar
            activeSection={section}
            onSectionChange={setSection}
            sampleCount={samples.length}
          />
        }
        main={renderMainContent()}
        panel={renderPanel()}
      />

      {/* Status Bar */}
      <StatusBar />

      {/* Keyboard Shortcuts Overlay */}
      <KeyboardShortcuts open={showShortcuts} onClose={() => setShowShortcuts(false)} />

      {/* Command Palette (Cmd+K) */}
      <CommandPalette
        isOpen={showCommandPalette}
        onClose={() => setShowCommandPalette(false)}
        actions={commands}
      />

      {/* Toast Notifications */}
      <ToastContainer />

      {/* Error Log Viewer */}
      <ErrorLogViewer open={showErrorLog} onClose={() => setShowErrorLog(false)} />

      {/* Cloud Storage Browser */}
      <CloudBrowser open={showCloudBrowser} onClose={() => setShowCloudBrowser(false)} />

      {/* New Project Modal */}
      <Modal
        open={showNewProject}
        onClose={() => setShowNewProject(false)}
        title="New Project"
        size="sm"
      >
        <div className="space-y-4">
          <div>
            <label className="label">Project Name</label>
            <input
              value={projectName}
              onChange={(e) => setProjectName(e.target.value)}
              placeholder="My Project"
              maxLength={255}
              className="input-field"
              autoFocus
              onKeyDown={(e) => {
                if (e.key === 'Enter') handleCreateProject();
              }}
            />
          </div>

          <div>
            <label className="label">Program Type</label>
            <div className="grid grid-cols-2 gap-2">
              <button
                onClick={() => setProgramType('Keygroup')}
                className={`p-3 rounded-lg border-2 text-center transition-all
                  ${programType === 'Keygroup'
                    ? 'border-accent-teal bg-accent-teal-50'
                    : 'border-border hover:border-accent-teal/30'
                  }`}
              >
                <div className="text-sm font-medium text-text-primary">Keygroup</div>
                <div className="text-[10px] text-text-muted mt-0.5">
                  Chromatic pitched instrument
                </div>
              </button>
              <button
                onClick={() => setProgramType('Drum')}
                className={`p-3 rounded-lg border-2 text-center transition-all
                  ${programType === 'Drum'
                    ? 'border-accent-plum bg-accent-plum-50'
                    : 'border-border hover:border-accent-plum/30'
                  }`}
              >
                <div className="text-sm font-medium text-text-primary">Drum</div>
                <div className="text-[10px] text-text-muted mt-0.5">
                  Pad-based drum kit
                </div>
              </button>
            </div>
          </div>

          {/* Starter Template */}
          <div>
            <label className="label">Starter Template</label>
            <div className="grid grid-cols-2 gap-2">
              {PROJECT_TEMPLATES.map((tpl) => (
                <button
                  key={tpl.id}
                  onClick={() => {
                    setSelectedTemplateId(selectedTemplateId === tpl.id ? null : tpl.id);
                    setProgramType(tpl.programType);
                  }}
                  className={`p-2 rounded-lg border-2 text-left transition-all
                    ${selectedTemplateId === tpl.id
                      ? 'border-accent-teal bg-accent-teal-50'
                      : 'border-border hover:border-accent-teal/30'
                    }`}
                >
                  <div className="text-sm">{tpl.emoji} {tpl.name}</div>
                  <div className="text-[9px] text-text-muted mt-0.5">{tpl.description}</div>
                </button>
              ))}
            </div>
          </div>

          <div className="flex justify-end gap-2 pt-2">
            <Button variant="secondary" onClick={() => setShowNewProject(false)}>
              Cancel
            </Button>
            <Button
              variant="primary"
              disabled={!projectName.trim()}
              onClick={handleCreateProject}
            >
              Create
            </Button>
          </div>
        </div>
      </Modal>
    </div>
  );
}
