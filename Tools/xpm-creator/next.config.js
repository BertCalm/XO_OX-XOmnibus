/** @type {import('next').NextConfig} */
const nextConfig = {
  output: 'export',
  // Tauri requires static export — disable image optimization (no server)
  images: {
    unoptimized: true,
  },
};

module.exports = nextConfig;
