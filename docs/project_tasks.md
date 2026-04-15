**1\. Which project did you choose?**

Vector Field Analysis \- Taking an input vector field and creating all the streamlines that flow across it. 

**2\. What tasks have you identified? What challenges do you foresee for each of those tasks?**

- Finding team meeting times that work with everyone   
- Making a plan so we don't have git merge conflicts   
- Figuring out how to accurately test our application outside the CHPC environment   
- Ensuring we have the same dependency versions for all our dev envs  
- Implement all the different required parallelized implementations  
- serial   
- parallel shared memory CPU   
- parallel CUDA GPU (no CUDA shared memory required)  
- distributed memory CPU  
- distributed memory GPU (with obviously some use of CPU code)  
- Handling vectors that point outside of the sub-section of the region handled by the separate parallel portions. 

**3\. How will the work be divided across the team?**

- Rylei will focus on parallelization and running the program on the supercomputer. Will implement CUDA, distributed memory GPU, and distributed memory CPU. Write the bash script for the Slurm functionality. Additionally, will analyze how different strategies affect the overall results.  
- Ethan will build the basic building blocks of the analysis. Namely, the vector, vector field, and streamline classes. As well as the necessary operations associated with them. This includes creating streamlines for various subsections and the reduction process for linking them. As well as the Shared Memory CPU implementation   
- Nate will make a vector field simulator so we can generate synthetic data for testing (rectangular, .ovf, .oompf, 2.0, 3.0). Make interfaces and runtime dependency injection so we can pick different implementations for our core algorithms. Unit, and integration testing ran with GitHub actions. Work to simulate the CHPC environment as closely as possible for local testing (maybe with a Slurm docker container). High quality documentation making it easy for anyone to reproduce our results. Implement the distributed memory CPU algorithm. Make a beautiful CLI interface. Help with everything else that needs to be done. If there is time: make the algorithms and data structures handle any dimension of space.   
  - [https://math.nist.gov/oommf/doc/userguide20b0/userguidexml/sec\_ovf20format.html](https://math.nist.gov/oommf/doc/userguide20b0/userguidexml/sec_ovf20format.html)   
  - [https://github.com/spirit-code/ovf](https://github.com/spirit-code/ovf) 

**4\. What is your timeline (keep in mind that scaling studies can take time, depending on how busy is the cluster)?**

- Week 1 (Mar 15 \- Mar 21): Form group, plan project timeline, divide tasks, figure out what project we are doing, discuss basic implementation, and how we could extend it if we have time.  
- Week 2 (Mar 22 \- Mar 28): Complete Sequential MVP, basic testing, CLI, and dependency injection system. Make the vector simulator.   
- Week 3 (Mar 29 \- Apr 4): Develop HPC implementations  
- Week 4 (Apr 5 \- Apr 11):  Extend for additional functionality as possible or refine current implementations. Work out any remaining issues.  
- Week 5 (Apr 12 \- Apr 18): Project Completion, write-up, and final analysis 

