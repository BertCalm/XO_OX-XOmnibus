import type { Metadata, Viewport } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'XPM Creator',
  description: 'Create MPC XPM programs and XPN expansion packs from audio samples',
  manifest: '/manifest.json',
  metadataBase: new URL('https://xo-ox.org'),
  openGraph: {
    title: 'XPM Creator',
    description: 'Professional MPC expansion pack creator',
    type: 'website',
    siteName: 'XO_OX Designs',
  },
  twitter: {
    card: 'summary_large_image',
  },
};

export const viewport: Viewport = {
  width: 'device-width',
  initialScale: 1,
  maximumScale: 1,
  themeColor: '#0D9488',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <body className="min-h-screen bg-surface-bg font-sans">
        {children}
      </body>
    </html>
  );
}
