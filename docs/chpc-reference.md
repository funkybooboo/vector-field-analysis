# CHPC Reference - University of Utah

Center for High Performance Computing
https://www.chpc.utah.edu

---

## Account & Access

- **Eligibility**: UoU students with a valid uNID. Requires a faculty sponsor (CHPC PI) - for CS6030, your instructor is the sponsor.
- **Account request**: https://www.chpc.utah.edu/role/user/account_request.php
- **CIS password** (used for SSH): http://gate.acs.utah.edu - this is separate from your Canvas/SSO/email password
- **2FA**: Duo Mobile is mandatory. Enroll at https://ese.idm.utah.edu/duo-management
- **SSH keys**: Not permitted. Must use password + Duo every login.
- **Account sharing**: Prohibited (University Policy 4-004)

---

## SSH Access

```bash
ssh <uNID>@notchpeak.chpc.utah.edu
```

**Auth flow:**
1. `Password:` - enter your CIS password (gate.acs.utah.edu)
2. `Passcode or option:` - enter `1` for Duo push, or paste the 6-digit code from Duo Mobile

**Off-campus**: SSH works directly without VPN. VPN is needed for some web resources but not SSH.

**Available clusters:**

| Cluster   | Hostname                      | Allocation Required | Notes                          |
|-----------|-------------------------------|---------------------|--------------------------------|
| notchpeak | notchpeak.chpc.utah.edu       | Yes                 | Primary cluster, A100 GPUs     |
| kingspeak | kingspeak.chpc.utah.edu       | No                  | Older hardware, good for free use |
| lonepeak  | lonepeak.chpc.utah.edu        | Account (`lonepeak-gpu`) | No InfiniBand, 1080Ti GPUs |
| redwood   | redwood.chpc.utah.edu         | Yes                 | Sensitive/health data workloads |
| granite   | granite.chpc.utah.edu         | Yes (GPU)           | Newest hardware, H100/L40S GPUs |

---

## Node Types

| Type            | Purpose                                                                 |
|-----------------|-------------------------------------------------------------------------|
| **Login node**  | Where you land after SSH (e.g. `notchpeak1`). For editing, compiling, and submitting jobs only. Do NOT run compute jobs here. |
| **Compute node**| Where batch and interactive jobs run. Requested via SLURM.             |
| **GPU node**    | Compute nodes with GPUs. Must specify `--gres=gpu:...` to access GPUs. |
| **Owner node**  | Nodes owned by specific research groups. Accessible as guest when idle. |
| **DTN**         | Data Transfer Nodes. Use for large file transfers - faster, bypasses campus firewall. |

---

## Cluster Hardware

### Notchpeak
Primary general-use cluster. **Requires allocation.**

| Node Type              | Count | CPUs                        | Cores | Memory       | GPU                    |
|------------------------|-------|-----------------------------|-------|--------------|------------------------|
| Intel Skylake          | 25    | Dual Intel Xeon Skylake     | 32    | 96-768 GB    | -                      |
| Intel Skylake (large)  | 1     | Dual Intel Xeon Skylake     | 36    | 768 GB       | -                      |
| Intel Cascadelake      | 7     | Dual Intel Xeon Cascadelake | 40    | 192 GB       | -                      |
| AMD Rome               | 32    | Single AMD EPYC Rome        | 64    | 256 GB       | -                      |
| AMD Naples             | 2     | AMD EPYC Naples             | 64    | 512 GB       | -                      |
| GPU nodes              | 9     | Intel Xeon                  | 32-40 | 192 GB       | P40, V100, A100, RTX2080Ti |

- **Interconnect**: Mellanox EDR InfiniBand
- **Notable partition**: `notchpeak-shared-short` - no allocation needed, max 8h walltime, 16 cores/128 GB per user

---

### Kingspeak
Older cluster. **No allocation required.**

| Node Type       | Count | CPUs                                  | Cores       | Memory      | GPU |
|-----------------|-------|---------------------------------------|-------------|-------------|-----|
| General nodes   | 385   | Intel Xeon (Sandy/Ivy/Haswell/Broadwell) | 16-32    | 32 GB-1 TB  | -   |
| GPU nodes       | 4     | Intel Xeon                            | varies      | varies      | Mixed |
| Interactive     | 2     | Intel Xeon                            | varies      | varies      | -   |

- **Total cores**: ~8,292
- **Interconnect**: Mellanox FDR InfiniBand + Gigabit Ethernet

---

### Lonepeak
No allocation required. No InfiniBand - not ideal for MPI/tightly-coupled jobs.

| Node Type     | Count | CPUs                              | Cores | Memory    | GPU               |
|---------------|-------|-----------------------------------|-------|-----------|-------------------|
| IvyBridge     | 48    | Intel Xeon E5-2697 v2             | 24    | 64 GB     | -                 |
| Nehalem       | 4     | Intel Xeon X7560                  | 32    | 512 GB-1 TB | -               |
| GPU nodes     | 21    | Intel Xeon E5-2620 v4 (Broadwell) | 16    | 128 GB    | 8x Nvidia 1080 Ti |
| Owner nodes   | 21    | varies                            | varies | varies   | varies            |

- **Interconnect**: Ethernet only (no InfiniBand)

---

### Redwood
Sensitive/health data cluster. **Requires allocation.**

| Node Type        | Count | CPUs                    | Cores | Memory    | GPU            |
|------------------|-------|-------------------------|-------|-----------|----------------|
| Intel Skylake    | 4     | Intel Xeon Skylake      | 32    | 128 GB    | -              |
| Intel Broadwell  | 9     | Intel Xeon Broadwell    | 28    | 128-256 GB | -             |
| AMD EPYC Rome    | 2     | AMD EPYC Rome           | 64    | 256 GB    | -              |
| Intel Nehalem    | 1     | Intel Xeon X7560        | 32    | 1 TB      | -              |
| GPU nodes        | 2     | Intel Xeon              | varies | varies   | 4x GTX 1080 Ti |

- **Interconnect**: Mellanox EDR (Skylake/Rome) + FDR (older) InfiniBand
- **Notable partition**: `redwood-shared-short` - no allocation, max 8h, 8 cores/32 GB per user

---

### Granite
Newest cluster, latest GPU hardware. **Requires allocation for GPU jobs.**

| Node Type            | Count | CPUs                | Cores | Memory   | GPU              |
|----------------------|-------|---------------------|-------|----------|------------------|
| GPU (H100 NVL)       | 1     | AMD EPYC Genoa      | 64    | 768 GB   | H100 NVL (96 GB) |
| GPU (L40S)           | 2     | AMD EPYC Genoa      | 64    | 384 GB   | L40S             |
| CPU single-socket    | 5     | AMD EPYC Genoa      | 96    | 768 GB   | -                |
| CPU dual-socket      | 3     | AMD EPYC Genoa      | 96    | 1536 GB  | -                |

- **Interconnect**: RDMA over Converged Ethernet at 100 Gb (no InfiniBand)
- **Freecycle partition**: `granite-gpu-freecycle` - preemptible, available to all users without allocation

---

## GPU Architecture Reference

| GPU                  | Architecture | CUDA SM | Memory   | Cluster(s)                        |
|----------------------|--------------|---------|----------|-----------------------------------|
| H200                 | Hopper       | sm_90   | 141 GB   | granite                           |
| H100 NVL             | Hopper       | sm_90   | 96 GB    | granite, notchpeak (owner/guest)  |
| GH200                | Hopper       | sm_90   | -        | granite (guest)                   |
| L40S                 | Ada Lovelace | sm_89   | 48 GB    | granite                           |
| L40                  | Ada Lovelace | sm_89   | 48 GB    | notchpeak (owner/guest)           |
| RTX 6000 Ada         | Ada Lovelace | sm_89   | 48 GB    | notchpeak (owner/guest)           |
| A100 80GB            | Ampere       | sm_80   | 80 GB    | notchpeak (owner/guest)           |
| A100                 | Ampere       | sm_80   | 40 GB    | notchpeak                         |
| A800                 | Ampere       | sm_80   | -        | notchpeak (owner/guest)           |
| A6000                | Ampere       | sm_86   | 48 GB    | notchpeak (owner/guest)           |
| A5500                | Ampere       | sm_86   | 24 GB    | notchpeak (owner/guest)           |
| A40                  | Ampere       | sm_86   | 48 GB    | notchpeak (owner/guest)           |
| RTX 3090             | Ampere       | sm_86   | 24 GB    | notchpeak                         |
| T4                   | Turing       | sm_75   | 16 GB    | notchpeak                         |
| RTX 2080 Ti          | Turing       | sm_75   | 11 GB    | notchpeak                         |
| Titan V              | Volta        | sm_70   | 12 GB    | notchpeak (owner/guest)           |
| V100                 | Volta        | sm_70   | 16/32 GB | notchpeak                         |
| P100                 | Pascal       | sm_60   | 16 GB    | kingspeak (guest)                 |
| GTX 1080 Ti          | Pascal       | sm_61   | 11 GB    | lonepeak, redwood                 |
| Titan X              | Pascal       | sm_61   | 12 GB    | kingspeak                         |
| P40                  | Pascal       | sm_61   | 24 GB    | notchpeak                         |

---

## Storage

| Path                                  | Quota | Notes                                                   |
|---------------------------------------|-------|---------------------------------------------------------|
| `~/`                                  | 50 GB | Home dir, shared across all clusters via GPFS. No backups. |
| `/scratch/general/vast/<uNID>/`       | None  | Fast shared scratch. Files >60 days old are auto-deleted. |
| `/scratch/local/`                     | None  | Node-local scratch. Fastest I/O. Lost when job ends.    |

- Home is **shared** - same files on notchpeak, kingspeak, granite, etc.
- Use scratch for large job I/O.

---

## Modules

```bash
module avail               # list all available modules
module avail cuda          # search for cuda modules
module load cuda           # load CUDA (required before nvcc)
module list                # show currently loaded modules
module unload cuda         # unload a module
module purge               # unload everything
```

---

## SLURM - Job Scheduler

All compute work goes through SLURM. Never run compute on login nodes.

### Check what you can run

```bash
mychpc batch               # list valid account/partition/QoS combinations for your user
freegpus                   # show currently idle GPUs by type and partition
sinfo                      # show all partition and node states
sprio -u <uNID>            # your current fair-share priority score
```

**Diagnosing pending jobs:**
- `(Priority)` in `squeue` means your job is eligible but waiting behind higher-priority jobs -- not an error.
- `(Resources)` means no node currently has the requested resources free.
- A job can show `(Priority)` even when `freegpus` shows idle GPUs of that type -- those GPUs may belong to owner partitions you don't have access to, or higher-priority jobs are already queued for them.
- Always check `freegpus` before submitting to confirm your target GPU type is actually available in your partition.

**Shell config gotcha:**
When storing remote paths containing `~` in a sourced `.env` file, use single quotes to prevent local shell expansion:
```bash
CHPC_PROJECT='~/projects/my-project'   # correct -- ~ expanded by remote shell
CHPC_PROJECT=~/projects/my-project     # wrong -- ~ expanded to local $HOME on source
```

### Interactive GPU session (development/testing)

```bash
srun --account=notchpeak-gpu \
     --partition=notchpeak-gpu \
     --gres=gpu:a100:1 \
     --time=1:00:00 \
     --pty bash
```

Without an allocation, use the shared-short partition (no account needed, max 8h):
```bash
srun --partition=notchpeak-shared-short \
     --gres=gpu:1 \
     --time=1:00:00 \
     --pty bash
```

### Batch job

```bash
sbatch job.sh
```

Example `job.sh`:
```bash
#!/bin/bash
#SBATCH --account=notchpeak-gpu
#SBATCH --partition=notchpeak-gpu
#SBATCH --gres=gpu:a100:1
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --time=0:30:00
#SBATCH --job-name=rgb_to_grey
#SBATCH --output=output/job_%j.log

module load cuda
./rgb_to_grey
```

**Important**: Always specify `--gres=gpu:...` or your job lands on a GPU node but has no GPU access.

### SLURM commands

```bash
squeue -u <uNID>           # your running/queued jobs
scancel <job_id>           # cancel a job
sacct -j <job_id>          # job accounting after completion
nvidia-smi                 # verify GPU allocation (run on compute node)
```

---

## Allocations

Allocations are free but require an application by a PI (faculty).
https://www.chpc.utah.edu/userservices/allocations.php

| Type       | Max CPU core-hours/quarter | Max GPU hours/quarter | Review           | Availability      |
|------------|----------------------------|-----------------------|------------------|-------------------|
| Small      | 30,000                     | 300                   | CHPC staff       | Immediately       |
| Regular    | 300,000                    | 3,000                 | Allocation Committee | Quarter start |

- Unused allocation does **not** carry over to the next quarter
- **Clusters requiring allocation**: notchpeak, redwood, granite (GPU)
- **No allocation needed**: kingspeak and `*-shared-short` partitions on notchpeak/redwood
- **Account required but no allocation committee review**: lonepeak (use account `lonepeak-gpu`)

---

## CUDA on CHPC

```bash
module avail cuda                                          # list available versions
module load cuda                                           # load default (currently 12.x)
module load cuda/12.3.2                                    # load a specific version
nvcc --version                                             # confirm loaded version
nvcc -arch=sm_80 -o my_program my_program.cu              # compile for A100 (notchpeak)
nvcc -arch=sm_80 -Xcompiler -Wall,-Werror -o my_program my_program.cu    # warnings as errors
```

**Minimum CUDA version by GPU target:**

| GPU         | SM arch | Min CUDA |
|-------------|---------|----------|
| H100 NVL    | sm_90   | 11.8     |
| L40S        | sm_89   | 11.8     |
| A100        | sm_80   | 11.0     |
| V100        | sm_70   | 9.0      |
| RTX 2080 Ti | sm_75   | 10.0     |
| GTX 1080 Ti | sm_61   | 8.0      |

**clang tools** (for formatting/linting):
```bash
module avail llvm                  # check available llvm/clang versions
module load llvm                   # load default; provides clang-format and clang-tidy
clang-format --version             # confirm (need 14.0+)
clang-tidy --version               # confirm (need 14.0+)
```

---

## Software Module Versions

Live catalog: https://portal.chpc.utah.edu/services/lmod-catalog/

### Compilers

| Software | Notable Versions | Notes |
|----------|-----------------|-------|
| GCC | 8.5.0, 9.2.0, 10.2.0, 11.2.0, 13.3.0, 13.4.0, 15.1.0 | 8.5.0 = system default (Rocky Linux 8) |
| Intel oneAPI | 2021.1.1, 2021.4.0 | Older: 2017–2019 also available |
| NVIDIA HPC SDK | 21.5 (all clusters), 22.1+ (notchpeak only) | Formerly PGI |

### Python

| Module | Versions |
|--------|----------|
| `python` | 3.10.3, 3.11.3, 3.11.7 |
| `anaconda3` | 2022.05, 2023.03 (+ older) |
| System `/usr/bin/python3` | 3.6.8 (limited, avoid for jobs) |

CHPC recommends installing your own Miniforge/Miniconda environment.

### CUDA & cuDNN

| Software | Available Versions | Newest |
|----------|-------------------|--------|
| CUDA | 6.0 – 13.2.0 (many) | 13.2.0 |
| cuDNN | 5.1 – 9.2.0.82 | 9.2.0.82 |

Notable recent CUDA: 11.8.0, 12.3.2, 12.6.3, 12.8.0, 12.8.1, 13.1.0, 13.2.0

### MPI

| Software | Notable Versions | Newest |
|----------|-----------------|--------|
| OpenMPI | 3.1.6, 4.1.1, 4.1.4, 4.1.6, 5.0.3, 5.0.7, 5.0.8 | 5.0.8 |
| OpenMPI (GPU-aware) | e.g. `openmpi/4.1.6-gpu` | — |
| MPICH | 3.2, 3.3.2 | 3.3.2 |
| Intel MPI | 2019.5, 2021.1.1, 2021.4.0, 2021.6.0 | 2021.6.0 |

### Deep Learning Meta-module

```bash
module load deeplearning/2025.4    # PyTorch 2.7.0, TF 2.19.0, JAX 0.6.0, CUDA 12.6.3
module load deeplearning/2024.2.0  # PyTorch 2.3.0, TF 2.16.1, JAX 0.4.28, CUDA 12.3
module load deeplearning/2023.3    # PyTorch 1.13.1, TF 2.11.0, CUDA 11.8
```

### Math & Libraries

| Software | Versions | Newest |
|----------|----------|--------|
| Intel MKL | 2019.x, 2021.x, 2024.2.2 | 2024.2.2 |
| OpenBLAS | 0.3.21, 0.3.25, 0.3.28 | 0.3.28 |
| FFTW | 3.3.6, 3.3.8, 3.3.10, 3.3.10-gpu | 3.3.10 |
| HDF5 | 1.10.7, 1.12.1, 1.14.3 | 1.14.3 |
| NetCDF | 4.8.1, 4.9.0 | 4.9.0 |
| Boost | 1.59.0 – 1.88.0 | 1.88.0 |
| CMake | 3.11.2 – 3.26.0 | 3.26.0 |

### Other Languages & Runtimes

| Software | Default/Notable Versions |
|----------|--------------------------|
| R | 4.1.3 (default) |
| Java (OpenJDK) | 11.0.5, 11.0.29 |
| Apptainer | 1.3.x – 1.4.1 |

---

## Accessing Without SSH

| Method        | URL                                                              |
|---------------|------------------------------------------------------------------|
| Open OnDemand | https://ondemand.chpc.utah.edu - terminal, file browser, Jupyter |
| FastX         | Web-based graphical desktop                                      |

---

## Support

| Channel      | Details                                                           |
|--------------|-------------------------------------------------------------------|
| Email        | helpdesk@chpc.utah.edu                                            |
| Phone        | 801-585-3791                                                      |
| Office hours | Most Fridays at 1:00 PM MT (virtual)                              |
| Duo issues   | Contact UIT - helpdesk@utah.edu (CHPC cannot help with Duo)       |
| Docs         | https://www.chpc.utah.edu/documentation/gettingstarted.php        |
| News         | https://www.chpc.utah.edu/news/latest_news/                       |

---

## Sources

Hardware/access content fetched from official CHPC documentation on 2026-03-24. Software module versions researched 2026-04-18.

| Topic                        | URL                                                                          |
|------------------------------|------------------------------------------------------------------------------|
| Module catalog (live)        | https://portal.chpc.utah.edu/services/lmod-catalog/                         |
| SSH access                   | https://www.chpc.utah.edu/documentation/software/ssh.php                    |
| Getting started              | https://www.chpc.utah.edu/documentation/gettingstarted.php                  |
| Accounts & eligibility       | https://www.chpc.utah.edu/userservices/accounts.php                         |
| Account policies             | https://www.chpc.utah.edu/documentation/policies/1.2AccountPolicies.php     |
| Duo 2FA                      | https://www.chpc.utah.edu/documentation/software/duo.php                    |
| Allocations                  | https://www.chpc.utah.edu/userservices/allocations.php                      |
| Notchpeak cluster            | https://www.chpc.utah.edu/documentation/guides/notchpeak.php                |
| Kingspeak cluster            | https://www.chpc.utah.edu/documentation/guides/kingspeak.php                |
| Lonepeak cluster             | https://www.chpc.utah.edu/documentation/guides/lonepeak.php                 |
| Redwood cluster              | https://www.chpc.utah.edu/documentation/guides/redwood.php                  |
| Granite cluster              | https://www.chpc.utah.edu/documentation/guides/granite.php                  |
| SLURM overview               | https://www.chpc.utah.edu/documentation/software/slurm.php                  |
| SLURM GPU jobs               | https://www.chpc.utah.edu/documentation/software/slurm-gpus.php             |
| GPUs & accelerators          | https://www.chpc.utah.edu/documentation/guides/gpus-accelerators.php        |
