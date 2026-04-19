# Running on CHPC

Step-by-step guide for running the vector field analysis pipeline on the University of Utah CHPC cluster.

---

## 1. One-time account setup

1. Request a CHPC account at https://www.chpc.utah.edu/role/user/account_request.php (requires a faculty sponsor — for CS6030, your instructor is the PI)
2. Set your CIS password at http://gate.acs.utah.edu (separate from Canvas/SSO)
3. Enroll in Duo Mobile at https://ese.idm.utah.edu/duo-management (mandatory 2FA)

---

## 2. One-time local setup

Copy `.env.example` to `.env` and fill in your values:

```sh
cp .env.example .env
```

Required fields:

```sh
# Your uNID
export CHPC_USER=u0000000

# Login node
export CHPC_HOST=notchpeak.chpc.utah.edu

# Path to the project on the remote (single quotes prevent local ~ expansion)
export CHPC_PROJECT='~/projects/vector-field-analysis'

# SLURM account and partition
export CHPC_ACCOUNT=notchpeak-gpu
export CHPC_PARTITION=notchpeak-gpu

# GPU type:count
export CHPC_GPU=a100:1

# Wall time limit
export CHPC_TIME=1:00:00

# CPU cores per job
export CHPC_NTASKS=4

# Modules (must match what is available on your target cluster)
export OPENMPI_MODULE=openmpi/5.0.8
export HDF5_MODULE=hdf5/1.14.6
export CUDA_MODULE=cuda/11.6.2
```

Check available module versions on CHPC with:
```sh
module spider openmpi
module spider hdf5
module spider cuda
```

---

## 3. Push code to CHPC

From your local machine:

```sh
./scripts/local/push.sh
```

This rsyncs everything except `build/`, `data/`, and `.env`.

---

## 4. Log in

```sh
./scripts/local/login.sh
```

Or directly:

```sh
ssh <uNID>@notchpeak.chpc.utah.edu
```

**Auth flow:**
1. `Password:` — enter your CIS password
2. `Passcode or option:` — enter `1` for a Duo push (approve on your phone)

---

## 5. First time on CHPC: create your `.env`

The `.env` file is not synced (it contains credentials). Create it once on CHPC:

```sh
cp .env.example .env
# edit .env and fill in your values
```

Then source it each session (or add to `~/.bashrc`):

```sh
source .env
```

---

## 6. Run the pipeline

Scripts load all required modules automatically — no manual `module load` needed.

```sh
# Submit all configs
./scripts/chpc/pipeline.sh

# Or submit specific stems
./scripts/chpc/pipeline.sh vortex_256x256 source_grid_divergent_512x512
```

This will:
1. Load openmpi, hdf5, and cuda modules
2. Configure and build both `simulator` and `analyzer` via CMake
3. Submit one SLURM batch job per config stem
4. Each job runs: `simulator` → `analyzer`
5. Output lands in `data/<stem>/`

---

## 7. Monitor jobs

```sh
# Show job queue + last 20 lines of all logs
./scripts/chpc/monitor.sh

# Live tail a specific stem
./scripts/chpc/monitor.sh vortex_256x256

# Raw SLURM queue
squeue -u $USER
```

---

## 8. Cancel jobs

```sh
# Cancel all pipeline jobs
./scripts/chpc/stop.sh

# Cancel one stem
./scripts/chpc/stop.sh vortex_256x256
```

---

## 9. Pull results back to local

From your local machine:

```sh
./scripts/local/pull.sh
```

This rsyncs `data/` from CHPC to local.

---

## 10. Visualize locally

```sh
mise run visualize:streams
```

---

## 11. Clean up on CHPC

```sh
make clean       # removes build/ and data/
```

---

## Interactive GPU session (debugging)

To get a shell on a GPU node for manual testing:

```sh
./scripts/chpc/gpu-shell.sh
```

Once on the compute node, modules are not inherited — load them manually:

```sh
source .env
source /etc/profile.d/lmod.sh
module load $OPENMPI_MODULE $HDF5_MODULE $CUDA_MODULE
```

Then run binaries directly:

```sh
./build/bins/simulator/simulator configs/vortex_256x256.toml
mpirun -n 4 ./build/bins/analyzer/analyzer configs/vortex_256x256.toml
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `OPENMPI_MODULE is not set` | Run `source .env` first |
| `Could NOT find HDF5` | Make sure `HDF5_MODULE` points to a version built with C++ support; use `module spider hdf5` to check |
| `error while loading shared libraries: libhdf5.so` | The compute node didn't load hdf5 — check `OPENMPI_MODULE` and `HDF5_MODULE` are set in `.env` |
| Job shows `(Priority)` in squeue | Normal — waiting behind higher-priority jobs. Check `freegpus` to confirm your GPU type is available |
| Job shows `(Resources)` in squeue | No node with the requested GPU is free right now |
| Want to check available GPUs | Run `freegpus` on the login node |
