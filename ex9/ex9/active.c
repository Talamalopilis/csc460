#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
/**
 * \file active.c
 * \brief A Skeleton Implementation of an RTOS
 * 
 * \mainpage A Skeleton Implementation of a "Full-Served" RTOS Model
 * This is an example of how to implement context-switching based on a 
 * full-served model. That is, the RTOS is implemented by an independent
 * "kernel" task, which has its own stack and calls the appropriate kernel 
 * function on behalf of the user task.
 *
 * \author Dr. Mantis Cheng
 * \date 29 September 2006
 *
 * ChangeLog: Modified by Alexander M. Hoole, October 2006.
 *			  -Rectified errors and enabled context switching.
 *			  -LED Testing code added for development (remove later).
 *
 * \section Implementation Note
 * This example uses the ATMEL AT90USB1287 instruction set as an example
 * for implementing the context switching mechanism. 
 * This code is ready to be loaded onto an AT90USBKey.  Once loaded the 
 * RTOS scheduling code will alternate lighting of the GREEN LED light on
 * LED D2 and D5 whenever the correspoing PING and PONG tasks are running.
 * (See the file "cswitch.S" for details.)
 */

//Comment out the following line to remove debugging code from compiled version.
// #define DEBUG

typedef void (*voidfuncptr)(void); /* pointer to void f(void) */

#define WORKSPACE 256
#define MAXPROCESS 4

/*===========
  * RTOS Internal
  *===========
  */

/**
  * This internal kernel function is the context switching mechanism.
  * It is done in a "funny" way in that it consists two halves: the top half
  * is called "Exit_Kernel()", and the bottom half is called "Enter_Kernel()".
  * When kernel calls this function, it starts the top half (i.e., exit). Right in
  * the middle, "Cp" is activated; as a result, Cp is running and the kernel is
  * suspended in the middle of this function. When Cp makes a system call,
  * it enters the kernel via the Enter_Kernel() software interrupt into
  * the middle of this function, where the kernel was suspended.
  * After executing the bottom half, the context of Cp is saved and the context
  * of the kernel is restore. Hence, when this function returns, kernel is active
  * again, but Cp is not running any more. 
  * (See file "switch.S" for details.)
  */
extern void CSwitch();
extern void Exit_Kernel(); /* this is the same as CSwitch() */

/* Prototype */
void Task_Terminate(void);

/** 
  * This external function could be implemented in two ways:
  *  1) as an external function call, which is called by Kernel API call stubs;
  *  2) as an inline macro which maps the call into a "software interrupt";
  *       as for the AVR processor, we could use the external interrupt feature,
  *       i.e., INT0 pin.
  *  Note: Interrupts are assumed to be disabled upon calling Enter_Kernel().
  *     This is the case if it is implemented by software interrupt. However,
  *     as an external function call, it must be done explicitly. When Enter_Kernel()
  *     returns, then interrupts will be re-enabled by Enter_Kernel().
  */
extern void Enter_Kernel();

#define Disable_Interrupt() asm volatile("cli" ::)
#define Enable_Interrupt() asm volatile("sei" ::)

/**
  *  This is the set of states that a task can be in at any given time.
  */
typedef enum process_states {
  DEAD = 0,
  READY,
  RUNNING
} PROCESS_STATES;

/**
  * This is the set of kernel requests, i.e., a request code for each system call.
  */
typedef enum kernel_request_type {
  NONE = 0,
  CREATE,
  NEXT,
  TERMINATE
} KERNEL_REQUEST_TYPE;

typedef enum priority_levels {
	ROUND_ROBIN = 0,
	PERIODIC,
	SYSTEM
} PRIORITY_LEVELS;

/**
  * Each task is represented by a process descriptor, which contains all
  * relevant information about this task. For convenience, we also store
  * the task's stack, i.e., its workspace, in here.
  */
typedef struct ProcessDescriptor
{
  unsigned char *sp; /* stack pointer into the "workSpace" */
  unsigned char workSpace[WORKSPACE];
  PROCESS_STATES state;
  PRIORITY_LEVELS priority;
  voidfuncptr code; /* function to be executed as a task */
  KERNEL_REQUEST_TYPE request;
} PD;

/**
  * This table contains ALL process descriptors. It doesn't matter what
  * state a task is in.
  */
static PD round_robin_tasks[MAXPROCESS];
static PD system_tasks[MAXPROCESS];
static PD periodic_tasks[MAXPROCESS];


/**
  * The process descriptor of the currently RUNNING task.
  */
volatile static PD *Cp;

/** 
  * Since this is a "full-served" model, the kernel is executing using its own
  * stack. We can allocate a new workspace for this kernel stack, or we can
  * use the stack of the "main()" function, i.e., the initial C runtime stack.
  * (Note: This and the following stack pointers are used primarily by the
  *   context switching code, i.e., CSwitch(), which is written in assembly
  *   language.)
  */
volatile unsigned char *KernelSp;

/**
  * This is a "shadow" copy of the stack pointer of "Cp", the currently
  * running task. During context switching, we need to save and restore
  * it into the appropriate process descriptor.
  */
volatile unsigned char *CurrentSp;

/** index to next task to run */
volatile static unsigned int NextP_RR;
volatile static unsigned int NextP_Per;
volatile static unsigned int NextP_Sys;

/** 1 if kernel has been started; 0 otherwise. */
volatile uint8_t KernelActive;

/** number of tasks created so far */
volatile static unsigned int Tasks;

/**
 * When creating a new task, it is important to initialize its stack just like
 * it has called "Enter_Kernel()"; so that when we switch to it later, we
 * can just restore its execution context on its stack.
 * (See file "cswitch.S" for details.)
 */
void Kernel_Create_Task_At(PD *p, voidfuncptr f)
{
  unsigned char *sp;

  //Changed -2 to -1 to fix off by one error.
  sp = (unsigned char *)&(p->workSpace[WORKSPACE - 1]);

  /*----BEGIN of NEW CODE----*/
  //Initialize the workspace (i.e., stack) and PD here!

  //Clear the contents of the workspace
  memset(&(p->workSpace), 0, WORKSPACE);

  //Notice that we are placing the address (16-bit) of the functions
  //onto the stack in reverse byte order (least significant first, followed
  //by most significant).  This is because the "return" assembly instructions
  //(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
  //second), even though the AT90 is LITTLE ENDIAN machine.

  //Store terminate at the bottom of stack to protect against stack underrun.
  *(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
  *(unsigned char *)sp-- = 0;

  //Place return address of function at bottom of stack
  *(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
  *(unsigned char *)sp-- = 0;

  //Place stack pointer at top of stack
  sp = sp - 33;

  p->sp = sp;  /* stack pointer into the "workSpace" */
  p->code = f; /* function to be executed as a task */
  p->request = NONE;

  /*----END of NEW CODE----*/

  p->state = READY;
}

/**
  *  Create a new task
  */
static void Kernel_Create_Task(voidfuncptr f, unsigned int priority)
{
  int x;
  PD * queue;
  
  switch (priority) {
		case ROUND_ROBIN:
			queue = round_robin_tasks;
			break;
		case PERIODIC:
			queue = periodic_tasks;
			break;
		case SYSTEM:
			queue = system_tasks;
			break;
		default:
			queue = round_robin_tasks;
			break;
  }

  if (Tasks == MAXPROCESS)
    return; /* Too many task! */

  /* find a DEAD PD that we can use  */
  for (x = 0; x < MAXPROCESS; x++)
  {
    if (queue[x].state == DEAD)
      break;
  }

  ++Tasks;
  Kernel_Create_Task_At(&(queue[x]), f);
}

/**
  * This internal kernel function is a part of the "scheduler". It chooses the 
  * next task to run, i.e., Cp.
  */
static void Dispatch()
{
  /* find the next READY task
       * Note: if there is no READY task, then this will loop forever!.
       */
	
  
  while (round_robin_tasks[NextP_RR].state != READY)
  {
    NextP_RR = (NextP_RR + 1) % MAXPROCESS;
  }

  Cp = &(round_robin_tasks[NextP_RR]);
  CurrentSp = Cp->sp;
  Cp->state = RUNNING;

  NextP_RR = (NextP_RR + 1) % MAXPROCESS;
}

/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Next_Kernel_Request()
{
  Dispatch(); /* select a new task to run */

  while (1)
  {
    Cp->request = NONE; /* clear its request */

    /* activate this newly selected task */
    CurrentSp = Cp->sp;
    Exit_Kernel(); /* or CSwitch() */

    /* if this task makes a system call, it will return to here! */

    /* save the Cp's stack pointer */
    Cp->sp = (unsigned char *)CurrentSp;

    switch (Cp->request)
    {
    case CREATE:
      Kernel_Create_Task(Cp->code, Cp->priority);
      break;
    case NEXT:
    case NONE:
      /* NONE could be caused by a timer interrupt */
      Cp->state = READY;
      Dispatch();
      break;
    case TERMINATE:
      /* deallocate all resources used by this task */
      Cp->state = DEAD;
      Dispatch();
      break;
    default:
      /* Houston! we have a problem here! */
      break;
    }
  }
}

/*================
  * RTOS  API  and Stubs
  *================
  */

/**
  * This function initializes the RTOS and must be called before any other
  * system calls.
  */
void OS_Init()
{
  int x;

  Tasks = 0;
  KernelActive = 0;
  NextP_RR = 0;
  NextP_Per = 0;
  NextP_Sys = 0;
  //Reminder: Clear the memory for the task on creation.
  for (x = 0; x < MAXPROCESS; x++)
  {
    memset(&(round_robin_tasks[x]), 0, sizeof(PD));
    round_robin_tasks[x].state = DEAD;
  }
}

/**
  * This function starts the RTOS after creating a few tasks.
  */
void OS_Start()
{
  if ((!KernelActive) && (Tasks > 0))
  {
    Disable_Interrupt();
    /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

    /* here we go...  */
    KernelActive = 1;
    Next_Kernel_Request();
    /* NEVER RETURNS!!! */
  }
}

  void setupTimer()
  {
    TCCR4A = 0;
    TCCR4B = 0;

    //set to CTC (mode 4)
    TCCR4B |= (1 << WGM32);

    //set prescaler to 256
    TCCR4B |= (1 << CS42);

    //set TOP value (0.01 seconds)
    OCR4A = 625;

    //Enable interrupt A for timer 3
    TIMSK4 |= (1 << OCIE4A);

    //Set timer to 0
    TCNT4 = 0;

    sei();
  }

  /**
  * For this example, we only support cooperatively multitasking, i.e.,
  * each task gives up its share of the processor voluntarily by calling
  * Task_Next().
  */
  void Task_Create_RR(voidfuncptr f)
  {
    if (KernelActive)
    {
      Disable_Interrupt();
      Cp->request = CREATE;
	  Cp->priority = ROUND_ROBIN;
      Cp->code = f;
      Enter_Kernel();
    }
    else
    {
      /* call the RTOS function directly */
      Kernel_Create_Task(f, ROUND_ROBIN);
    }
  }

  /**
  * The calling task gives up its share of the processor voluntarily.
  */
  void Task_Next()
  {
    if (KernelActive)
    {
      Disable_Interrupt();
      Cp->request = NEXT;
      Enter_Kernel();
    }
  }

  /**
  * The calling task terminates itself.
  */
  void Task_Terminate()
  {
    if (KernelActive)
    {
      Disable_Interrupt();
      Cp->request = TERMINATE;
      Enter_Kernel();
      /* never returns here! */
    }
  }

  // NOTE: do not touch this code
  // adding pretty much any line of code to this ISR
  // will break the entire program
  ISR(TIMER4_COMPA_vect)
  {
    asm("jmp Enter_Kernel_Interrupt" ::);
  }

  /*============
  * A Simple Test 
  *============
  */

  void Ping()
  {
    int x, y;
    DDRB = 0xff;
    for (;;)
    {
      //LED on
      PORTB = 0xff;

      for (y = 0; y < 32; ++y)
      {
        for (x = 0; x < 32000; ++x)
        {
          asm("");
        }
      }

      //LED off
      PORTB = 0;

      for (y = 0; y < 32; ++y)
      {
        for (x = 0; x < 32000; ++x)
        {
          asm("");
        }
      }

      /* printf( "*" );  */
    }
  }

  /**
  * A cooperative "Pong" task.
  * Added testing code for LEDs.
  */
  void Pong()
  {
    int x, y;
    DDRC = 0xff;
    for (;;)
    {
      //LED on
      PORTC = 0xff;
      for (y = 0; y < 64; ++y)
      {
        for (x = 0; x < 32000; ++x)
        {
          asm("");
        }
      }

      //LED off
      PORTC = 0;

      for (y = 0; y < 64; ++y)
      {
        for (x = 0; x < 32000; ++x)
        {
          asm("");
        }
      }

      /* printf( "." );  */
    }
  }

  /**
  * This function creates two cooperative tasks, "Ping" and "Pong". Both
  * will run forever.
  */
  int main()
  {
    OS_Init();
    Task_Create_RR(Pong);
    Task_Create_RR(Ping);
    setupTimer();
    OS_Start();
  }
