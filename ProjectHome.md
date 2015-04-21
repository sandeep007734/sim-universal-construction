05/11/2013: New version 1.4 for **Sim** and **Synch** synchronization techniques

> Bug fixes and performance improvements in some machine architectures


26/11/2012: New version 1.2 for **Sim** and **Synch** synchronization techniques

02/11/2011: The code of **Synch Synchronization Techniques** is uploaded

13/11/2011: New Version 0.9 of **Sim Universal Construction**

---


Sim is a simple wait-free universal construction, that uses just a Fetch&Add and a CAS object and performs a constant number of shared memory accesses. We have implemented Sim in a real shared-memory machine. In theory terms, our practical version of Sim, called P-Sim,
has worse complexity than its theoretical analog; in practice though, P-Sim outperforms
several state-of-the-art lock-based and lock-free techniques, and this given that it is wait-free, i.e., that it satisfies a stronger progress condition than all the algorithms it outperforms. We have used P-Sim to get highly-efficient wait-free implementations of stacks and queues. Our experiments show that our implementations outperform the currently state-of-the-art shared stack and queue implementations which ensure only weaker progress properties than wait-freedom.

The source code is distributed under the **New BSD License**.
By downloading the source code you accept the License.

Copyright (c) 2011, Nikolaos D. Kallimanis
All rights reserved.