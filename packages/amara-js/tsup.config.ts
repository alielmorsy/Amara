import { defineConfig } from 'tsup';

export default defineConfig([
  // Main bundle (core + runtime)
  {
    entry: ['src/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    clean: true,
    minify: true,
    splitting: false,
    outDir: 'dist',
  },
  // Core package (platform-agnostic)
  {
    entry: ['src/core/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    minify: true,
    outDir: 'dist/core',
  },
  // Runtime package
  {
    entry: ['src/runtime/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    minify: true,
    outDir: 'dist/runtime',
  },
  // Web runtime package
  {
    entry: ['src/runtime/web/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    minify: true,
    outDir: 'dist/runtime/web',
  },
  // Native runtime package
  {
    entry: ['src/runtime/native/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    minify: true,
    outDir: 'dist/runtime/native',
  },
  // Runtime exports for web
  {
    entry: ['runtime/web/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    minify: true,
    outDir: 'dist/runtime/web',
    noExternal: [/(.*)/],
  },
  // Runtime exports for native
  {
    entry: ['runtime/native/index.ts'],
    format: ['cjs', 'esm'],
    dts: true,
    sourcemap: true,
    minify: true,
    outDir: 'dist/runtime/native',
    noExternal: [/(.*)/],
  },
]);
