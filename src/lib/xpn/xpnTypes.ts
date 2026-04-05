export interface XpnMetadata {
  title: string;
  identifier: string;
  description: string;
  creator: string;
  tags: string[];
  version: string;
}

export interface XpnProgram {
  name: string;
  xpmContent: string;
  groupName: string;
  subName: string;
}

export interface XpnSample {
  fileName: string;
  data: ArrayBuffer;
}

export interface XpnPackageConfig {
  metadata: XpnMetadata;
  programs: XpnProgram[];
  samples: XpnSample[];
  coverArt?: {
    data: ArrayBuffer;
    type: 'jpg' | 'png';
  };
  previewAudio?: {
    data: ArrayBuffer;
    type: 'wav' | 'mp3';
  };
}
