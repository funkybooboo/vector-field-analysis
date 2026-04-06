# Simulator Configs

Ready-to-run TOML configs for the simulator. Each file is fully self-contained
and documented with the physics behind it.

## Running a config

```sh
simulator bins/simulator/configs/<name>.toml
mise run visualize
```

## Config file format

All settings live under a `[simulation]` table. Field layers are `[[layers]]`
array entries and are summed by superposition. See
[`bins/simulator/README.md`](../README.md) for the full schema reference.

---

## Demos

Simple fields that demonstrate individual layer types and the config system.

### `vortex.toml`

Single counter-clockwise vortex. Decays via viscosity.

```sh
simulator bins/simulator/configs/vortex.toml
```

### `spiral.toml`

Inward spiral -- vortex rotation blended 70% toward a sink. Viscous decay
makes the field fade to zero over time.

```sh
simulator bins/simulator/configs/spiral.toml
```

### `dual_vortex.toml`

Two counter-rotating vortices side by side. The left spins CCW (strength +1),
the right CW (strength -1). Both decay via viscosity.

```sh
simulator bins/simulator/configs/dual_vortex.toml
```

### `source_sink.toml`

A source on the left and a sink on the right. Flow radiates from the source,
crosses the domain, and converges into the sink. A noise layer adds turbulent
disturbance and viscous decay weakens the field over time.

```sh
simulator bins/simulator/configs/source_sink.toml
```

### `noise.toml`

Pure Perlin noise field. No symmetry, organic-looking flow. Good for
stress-testing the analyzer with irregular input.

```sh
simulator bins/simulator/configs/noise.toml
```

### `layered.toml`

Three layers combined: a central vortex, an off-center source, and
low-amplitude noise. Demonstrates how layers compose into complex fields.

```sh
simulator bins/simulator/configs/layered.toml
```

### `custom_rotation.toml`

Solid-body rotation that oscillates in speed with `cos(0.5*t)` -- the field
slows, reverses, then spins back up. Demonstrates the `custom` layer type and
use of the `t` variable in expressions.

```sh
simulator bins/simulator/configs/custom_rotation.toml
```

### `static.toml`

Zero-velocity field. Every vector is zero everywhere, no time variation.
Useful as a baseline or for testing tools that need a valid but trivial input.

```sh
simulator bins/simulator/configs/static.toml
```

### `pinwheel.toml`

Four sources on the cardinal axes and four sinks on the diagonals, all at
radius 1.0. Flow spirals outward from each source into the two adjacent sinks,
creating a pinwheel pattern.

```sh
simulator bins/simulator/configs/pinwheel.toml
```

### `circular_flow.toml`

Flow confined to a ring of radius 1.0 -- a circular conveyor belt. A Gaussian
radial envelope keeps the field near-zero inside and outside the ring.

```sh
simulator bins/simulator/configs/circular_flow.toml
```

### `boundary_layer.toml`

No-slip boundary layer along the bottom wall (`y = -2`). A `tanh` velocity
profile rises from zero at the wall to near-maximum in the free stream,
analogous to a Blasius boundary layer.

```sh
simulator bins/simulator/configs/boundary_layer.toml
```

### `spiral_galaxy.toml`

Differential rotation (`omega ~ 1/r`) with a weak inward drift. Inner orbits are
faster than outer ones, as in a real galactic disk. The small infall term adds
a slow radial inward drift.

```sh
simulator bins/simulator/configs/spiral_galaxy.toml
```

### `jet_stream.toml`

A focused high-speed core (tight Gaussian band) with superimposed turbulent
noise, modelling an atmospheric jet stream. The noise layer adds realistic
meandering and eddies.

```sh
simulator bins/simulator/configs/jet_stream.toml
```

### `simple_kelvin_helmholtz.toml`

Minimal Kelvin-Helmholtz setup: two opposing streams meeting at `y = 0` plus a
sinusoidal transverse perturbation at the interface. Shows the initial cat's-eye
instability structure without the exponential growth of the full `kelvin_helmholtz.toml`.

```sh
simulator bins/simulator/configs/simple_kelvin_helmholtz.toml
```

---

## Stream patterns

Configs focused on isolated and interacting stream flows.

### `one_stream.toml`

Single left-to-right stream centered in the domain. Wider Gaussian envelope
than the multi-stream configs so the band is clearly visible.

```sh
simulator bins/simulator/configs/one_stream.toml
```

### `two_streams.toml`

Two parallel left-to-right streams at `y = +/-0.8`. Gaussian envelopes keep
them non-touching. No viscous decay -- steady over all steps.

```sh
simulator bins/simulator/configs/two_streams.toml
```

### `three_streams.toml`

Three evenly spaced left-to-right streams at `y = -1.2, 0.0, +1.2`.

```sh
simulator bins/simulator/configs/three_streams.toml
```

### `four_streams.toml`

Four evenly spaced left-to-right streams at `y = -1.5, -0.5, +0.5, +1.5`.
Tighter Gaussian width (0.04) keeps neighbors non-touching at 1.0 unit spacing.

```sh
simulator bins/simulator/configs/four_streams.toml
```

### `different_speeds.toml`

Three streams at the same Gaussian width but different speeds: top at full
strength (1.0), middle at half (0.5), bottom at slowest (0.2). Makes speed
differences immediately visible in the arrow coloring.

```sh
simulator bins/simulator/configs/different_speeds.toml
```

### `opposing_streams.toml`

Upper stream flows right, lower stream flows left. A shear layer forms at
`y = 0`. This is the unperturbed setup that leads to Kelvin-Helmholtz instability
when a transverse perturbation is added.

```sh
simulator bins/simulator/configs/opposing_streams.toml
```

### `crossing_streams.toml`

A horizontal stream and a vertical stream intersect at the origin. Each is
Gaussian-confined to its axis. The interference region at the crossing shows
vector superposition.

```sh
simulator bins/simulator/configs/crossing_streams.toml
```

### `converging_streams.toml`

Two sources (upper-left and lower-left) feeding a single sink on the right.
The two flows approach from different angles and merge as they converge.

```sh
simulator bins/simulator/configs/converging_streams.toml
```

### `splitting_stream.toml`

One source on the left bifurcates into two sinks on the upper-right and
lower-right. Flow enters as a unified stream then splits.

```sh
simulator bins/simulator/configs/splitting_stream.toml
```

### `river_delta.toml`

One source fans out into three sinks, like a river delta splitting into
distributary channels. The central channel flows straight across; upper and
lower channels diverge at an angle.

```sh
simulator bins/simulator/configs/river_delta.toml
```

### `drifting_stream.toml`

A single stream that oscillates vertically over time, sweeping up and down
through the domain while always flowing rightward.

```sh
simulator bins/simulator/configs/drifting_stream.toml
```

### `pulsing_stream.toml`

A single stream that smoothly pulses on and off via a `sin(t)` modulation.
One full cycle takes ~126 steps at the default `dt = 0.05`.

```sh
simulator bins/simulator/configs/pulsing_stream.toml
```

### `swapping_streams.toml`

Two streams oscillating in opposite vertical phases, continuously swapping
positions. They pass through each other at regular intervals.

```sh
simulator bins/simulator/configs/swapping_streams.toml
```

### `stream_with_vortex.toml`

Two parallel streams with a vortex sitting at their interface. The vortex
deflects both streams and creates a mixing region between them.

```sh
simulator bins/simulator/configs/stream_with_vortex.toml
```

### `multi_flow_2.toml`

Two uniform background streams at different angles, plus two source-sink pairs
driving parallel channels at `y = +/-0.7`.

```sh
simulator bins/simulator/configs/multi_flow_2.toml
```

### `multi_flow_3.toml`

Three uniform streams plus three source-sink channel pairs at `y = -1.2, 0.0, +1.2`.

```sh
simulator bins/simulator/configs/multi_flow_3.toml
```

### `multi_flow_4.toml`

Four uniform streams plus four source-sink channel pairs spanning the full
vertical extent of the domain.

```sh
simulator bins/simulator/configs/multi_flow_4.toml
```

---

## Real-world simulations

### `hurricane.toml`

Tropical cyclone (Northern Hemisphere). Three-layer model:

- Core spiral with 12% inward convergence (matches real surface inflow angle)
- Perlin noise for outer rainband turbulence
- Weak uniform steering flow on a NW track

Viscous decay simulates gradual weakening.

```sh
simulator bins/simulator/configs/hurricane.toml
```

### `karman_street.toml`

Von Karman vortex street -- the alternating vortex pattern that forms
downstream of any bluff body (bridge pier, smokestack, island, aircraft wing).

Uses the **exact analytical formula** for two infinite periodic rows of point
vortices (Lamb's *Hydrodynamics*). The rows advect rightward in real time.
The implied cylinder is off the left edge of the domain.

```sh
simulator bins/simulator/configs/karman_street.toml
```

### `wind_tunnel.toml`

Potential flow around a circular cylinder. Combines a uniform free stream with
a doublet at the origin -- the exact irrotational solution. The free stream
pulses with `sin(0.3*t)` to simulate gusts. The cylinder boundary (r = 0.3)
is visible where arrows reverse inside the solid body.

```sh
simulator bins/simulator/configs/wind_tunnel.toml
```

### `accretion_disk.toml`

Black hole / neutron star accretion disk. Keplerian orbital speed
(`v ~ 1/sqrt(r)`) combined with ~20% radial infall, driven by
angular momentum transport (magneto-rotational instability in real disks).
A noise layer approximates MRI-driven turbulence in the disk midplane.

```sh
simulator bins/simulator/configs/accretion_disk.toml
```

### `kelvin_helmholtz.toml`

Kelvin-Helmholtz shear instability. Occurs at any interface where two fluid
layers slide past each other: ocean-atmosphere boundary, jet stream edges,
cloud formations, Jupiter's bands, stellar interiors.

A `tanh` shear profile is perturbed by a sinusoidal transverse displacement
that grows exponentially with time (`exp(0.12*t)`), mimicking the linear
instability growth rate. By the final steps the perturbation rolls up into
billows.

```sh
simulator bins/simulator/configs/kelvin_helmholtz.toml
```

### `magnetosphere.toml`

Earth's magnetosphere interacting with the solar wind. Viewed in the
noon-midnight meridional plane (cross-section through the magnetic poles).

Earth's dipole uses two 1/r^2-decaying custom poles. The magnetopause
(standoff at ~r = 1.0) is visible as the boundary where the solar wind and
dipole pressures balance. The solar wind pulses with `sin(0.4*t)`, simulating
pressure variations from a passing coronal mass ejection.

```sh
simulator bins/simulator/configs/magnetosphere.toml
```

### `pulsatile_flow.toml`

Pulsatile blood flow in a large artery (coronary or aorta). The parabolic
Poiseuille profile satisfies the no-slip condition at the walls (y = +/-1).
The amplitude cycles at 1.2 rad/s (~72 bpm resting heart rate), including
a brief flow reversal during diastole -- physiologically normal in the
coronary arteries.

```sh
simulator bins/simulator/configs/pulsatile_flow.toml
```
