# Online Judger (base on: Loco program runner core -> WeJudge Judger)

Our judger is forked from the "WeJudge Judger" which is based on Loco program runner core.

We modified the way to get used memory for the child process in **Not-tracing** cases.
Instead of get it from `rusage.maxrss` (which is incorrect sometimes), we get it from the `VmPeak` field (which implies the maximum size of virtual memory) from `/proc/pid/status`. In order to do that, we use `ptrace` to intercept the system call `exit` and `exit_group`, and read the file `/proc/pid/status`, because it will be deleted when the child process finished.
Correspondingly, we limit the memory use by limiting `RLIMIT_AS` of `setrlimit`, instead of limiting `RLIMIT_DATA` (only the size of data segment) which cannot stop the `mmap` system call.


> (From Loco program runner core) 
> We use this python-c library to run program in a sandbox-like environment.
With it, we can accurately known the resource using of the program and 
limit its resource using including system-call interrupt.
>
>
>## Usage
>
>
>For run a program without tracing:
>```python
>runcfg = {
>    'args':['./m'], # args are split by " " into a list of strings
>    'fd_in':fin.fileno(),
>    'fd_out':ftemp.fileno(),
>    'timelimit':1000, #in MS
>    'memorylimit':20000, #in KB
>}
>
>rst = lorun.run(runcfg)
>```
>For check one output:
>```python
>ftemp = file('temp.out')
>fout = file(out_path)
>crst = lorun.check(fout.fileno(), ftemp.fileno())
>```
>
>## Trace
>You can set runcfg['trace'] to True to ensure runner's security.
>Here is a simple usage:
>
>```python
>runcfg['trace'] = True
>runcfg['calls'] = [1, 2, 3, 4] # system calls that could be used by testing programs
>runcfg['files'] = {'/etc/ld.so.cache': 0} # open flag permitted (value is the flags of open)
>```