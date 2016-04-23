<div id="table-of-contents">
<h2>Table of Contents</h2>
<div id="text-table-of-contents">
<ul>
<li><a href="#sec-1">1. Lab3</a>
<ul>
<li><a href="#sec-1-1">1.1. <span class="done DONE">DONE</span> 练习0 ： 填写已有实验 <code>[1/1]</code></a></li>
<li><a href="#sec-1-2">1.2. <span class="todo TODO">TODO</span> 练习1: 给未被映射的地址映射上物理页（需要编程）</a></li>
<li><a href="#sec-1-3">1.3. <span class="todo TODO">TODO</span> 练习2: 补充完成基于FIFO的页面替换算法（需要编程）</a></li>
</ul>
</li>
</ul>
</div>
</div>

# Lab3<a id="sec-1" name="sec-1"></a>

## DONE 练习0 ： 填写已有实验 <code>[1/1]</code><a id="sec-1-1" name="sec-1-1"></a>

本实验依赖实验1/2.请把你的实验1/2的代码填入本实验中代码中有“LAB1”,"LAB2"的注释相应部份。
-   [X] 用meld工具来与LAB2完成的代码进行代码合并

## TODO 练习1: 给未被映射的地址映射上物理页（需要编程）<a id="sec-1-2" name="sec-1-2"></a>

完成do_pgfault(mm/vmm.c)函数，给未被映射的地址映射上物理页。设置访问权限的时候需要参考
页面所在VMA的权限，同时需要注意映射物理页时需要操作内存控制结构所指定的页表，而不是内核的页
表。注意：在LAB3 EXERCISE 1处填写代码。执行

    make qemu

后，如果通过check_pgfault函数的测试后，会有“check_pgfault() succeeded”的输出，表示
练习1基本正确请在实验报告中简要说明你的设计实现过程。请回答如下问题：
＋ 请描述页目录(Page Directory Entry)和页表(Page Table Entry)中组成部分对ucore实现
页替换算法的潜在用处
＋ 如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？
-   [ ] 说明设计实现过程

```c
      1  /* do_pgfault - interrupt handler to process the page fault execption
      2   * @mm         : the control struct for a set of vma using the same PDT
      3   * @error_code : the error code recorded in trapframe->tf_err which is setted by x86 hardware
      4   * @addr       : the addr which causes a memory access exception, (the contents of the CR2 register)
      5   *
      6   * CALL GRAPH: trap--> trap_dispatch-->pgfault_handler-->do_pgfault
      7   * The processor provides ucore's do_pgfault function with two items of information to aid in diagnosing
      8   * the exception and recovering from it.
      9   *   (1) The contents of the CR2 register. The processor loads the CR2 register with the
     10   *       32-bit linear address that generated the exception. The do_pgfault fun can
     11   *       use this address to locate the corresponding page directory and page-table
     12   *       entries.
     13   *   (2) An error code on the kernel stack. The error code for a page fault has a format different from
     14   *       that for other exceptions. The error code tells the exception handler three things:
     15   *         -- The P flag   (bit 0) indicates whether the exception was due to a not-present page (0)
     16   *            or to either an access rights violation or the use of a reserved bit (1).
     17   *         -- The W/R flag (bit 1) indicates whether the memory access that caused the exception
     18   *            was a read (0) or write (1).
     19   *         -- The U/S flag (bit 2) indicates whether the processor was executing at user mode (1)
     20   *            or supervisor mode (0) at the time of the exception.
     21   */
     22   int
     23   do_pgfault(struct mm_struct *mm, uint32_t error_code, uintptr_t addr) {
     24       int ret = -E_INVAL;
     25       //try to find a vma which include addr
     26       struct vma_struct *vma = find_vma(mm, addr);
     27  
     28       pgfault_num++;
     29       //If the addr is in the range of a mm's vma?
     30       if (vma == NULL || vma->vm_start > addr) {
     31         cprintf("not valid addr %x, and  can not find it in vma\n", addr);
     32         goto failed;
     33       }
     34       //check the error_code
     35       switch (error_code & 3) {
     36       default:
     37         /* error code flag : default is 3 ( W/R=1, P=1): write, present */
     38       case 2: /* error code flag : (W/R=1, P=0): write, not present */
     39         if (!(vma->vm_flags & VM_WRITE)) {
     40             cprintf("do_pgfault failed: error code flag = write AND not present, but the addr's vma cannot write\n");
     41             goto failed;
     42         }
     43       break;
     44       case 1: /* error code flag : (W/R=0, P=1): read, present */
     45         cprintf("do_pgfault failed: error code flag = read AND present\n");
     46         goto failed;
     47       case 0: /* error code flag : (W/R=0, P=0): read, not present */
     48         if (!(vma->vm_flags & (VM_READ | VM_EXEC))) {
     49             cprintf("do_pgfault failed: error code flag = read AND not present, but the addr's vma cannot read or exec\n");
     50             goto failed;
     51             }
     52       }
     53       /* IF (write an existed addr ) OR
     54       *    (write an non_existed addr && addr is writable) OR
     55       *    (read  an non_existed addr && addr is readable)
     56       * THEN
     57       *    continue process
     58       */
     59  
     60       uint32_t perm = PTE_U;
     61       if (vma->vm_flags & VM_WRITE) {
     62         perm |= PTE_W;
     63       }
     64       addr = ROUNDDOWN(addr, PGSIZE);
     65  
     66       ret = -E_NO_MEM;
     67  
     68       pte_t *ptep=NULL;
     69       /*LAB3 EXERCISE 1: YOUR CODE
     70       * Maybe you want help comment, BELOW comments can help you finish the code
     71       *
     72       * Some Useful MACROs and DEFINEs, you can use them in below implementation.
     73       * MACROs or Functions:
     74       *   get_pte : get an pte and return the kernel virtual address of this pte for la
     75       *             if the PT contians this pte didn't exist, alloc a page for PT (notice the 3th parameter '1')
     76       *   pgdir_alloc_page : call alloc_page & page_insert functions to allocate a page size memory & setup
     77       *             an addr map pa<--->la with linear address la and the PDT pgdir
     78       * DEFINES:
     79       *   VM_WRITE  : If vma->vm_flags & VM_WRITE == 1/0, then the vma is writable/non writable
     80       *   PTE_W           0x002                   // page table/directory entry flags bit : Writeable
     81       *   PTE_U           0x004                   // page table/directory entry flags bit : User can access
     82       * VARIABLES:
     83       *   mm->pgdir : the PDT of these vma
     84       *
     85       */
     86     //#if 0
     87       /*LAB3 EXERCISE 1: YOUR CODE*/
     88       ptep = get_pte(mm->pgdir, addr, 1);              //(1) try to find a pte, if pte's PT(Page Table) isn't existed, then create a PT.
     89       if (ptep == NULL) {
     90         cprintf("get_pte failed in do_pgfault \n");
     91         goto failed;
     92       }
     93       //(2) if the phy addr isn't exist, then alloc a page & map the phy addr with logical addr
     94       if (*ptep == 0) {
     95         if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
     96             cprintf("pgdir_alloc_page failed in do_pgfault \n");
     97             goto failed;
     98         }
     99       }
    100       else {
    101       /*LAB3 EXERCISE 2: YOUR CODE
    102       * Now we think this pte is a  swap entry, we should load data from disk to a page with phy addr,
    103       * and map the phy addr with logical addr, trigger swap manager to record the access situation of this page.
    104       *
    105       *  Some Useful MACROs and DEFINEs, you can use them in below implementation.
    106       *  MACROs or Functions:
    107       *    swap_in(mm, addr, &page) : alloc a memory page, then according to the swap entry in PTE for addr,
    108       *                               find the addr of disk page, read the content of disk page into this memroy page
    109       *    page_insert ： build the map of phy addr of an Page with the linear addr la
    110       *    swap_map_swappable ： set the page swappable
    111       */
    112         if(swap_init_ok) {
    113             struct Page *page=NULL;
    114                                  //(1）According to the mm AND addr, try to load the content of right disk page
    115                                  //    into the memory which page managed.
    116             int r;
    117             r = swap_in(mm, addr, &page);
    118             if (r != 0) {
    119                 cprintf("swap_in failed in do_pgfault \n");
    120                 goto failed;
    121             }
    122                                  //(2) According to the mm, addr AND page, setup the map of phy addr <---> logical addr
    123             page_insert(mm->pgdir, page, addr, perm);
    124                                  //(3) make the page swappable.
    125             swap_map_swappable(mm, addr, page, 1);
    126             page->pra_vaddr = addr;
    127         }
    128         else {
    129             cprintf("no swap_init_ok but ptep is %x, failed\n",*ptep);
    130             goto failed;
    131         }
    132     }
    133     //#endif
    134     ret = 0;
    135     failed:
    136         return ret;
    137 }
```

-   [X] 回答问题1

-   [ ] 回答问题2

## TODO 练习2: 补充完成基于FIFO的页面替换算法（需要编程）<a id="sec-1-3" name="sec-1-3"></a>
