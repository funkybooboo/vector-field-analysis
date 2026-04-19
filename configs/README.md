# configs

TOML configuration files for the simulator and analyzer. Each file defines one named scenario.

| Config | Description |
|--------|-------------|
| [`karman_street_128x64.toml`](karman_street_128x64.toml) | Von Kármán vortex street — alternating vortices downstream of a bluff body |
| [`karman_street_128x64_cuda.toml`](karman_street_128x64_cuda.toml) | Same scenario configured for the CUDA analyzer solver |
| [`source_grid_divergent_512x512.toml`](source_grid_divergent_512x512.toml) | Five uniform background flows at varied angles on a 512×512 grid |

## Running

```sh
# Run a single config through the full pipeline
scripts/pipeline.sh karman_street_128x64

# Run all configs
scripts/pipeline.sh
```

Output is written to `data/<stem>/` for each config stem.
