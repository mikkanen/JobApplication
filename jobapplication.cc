/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
//  jobapplication.cc                                                                          //
//  An alternative job application                                                             //
//                                                                                             //
//  When you start the program it ask password, correct password is "password". :-)            //
//  The program creates project manager which follows agile methologies and, who have 12       //
//  software developers as resource. The project manager creates and sends tasks to            //
//  software developers via the FIFO queue. Software developers listen to the FIFO queue and   //
//  the fastest one gets it for caried out.                                                    //
//                                                                                             //
//  Each software developer and project manager lives in two threads (thread).                 //
//  In the second thread, MammalBasicFunctions_c provides basic functions such as breathing,   //
//  eating, sleeping. So, while sleeping or eating not do any work. The first thread          //
//  carries out the actual work(payload).                                                      //
//                                                                                             //
//  (Total project manager and 12 software developers mean 26 threads((12*2)+2)).              //
//                                                                                             //
//                                                                                             //
//  In the below are instructions for compiling the program in the target system.              //
//  This program is coded on Fedora 27 Linux-system, but it most probably can be compiled      //
//  and executed on ,e.g., MacOS. It is standard STL program and it follows C++11 standard.    //
//                                                                                             //
//  Created by Markku Mikkanen on 30/04/2018.                                                  //
//  Copyright © 2018 Markku Mikkanen. All rights reserved.                                     //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

// Compile instructions:
// []$> g++ -Wall -std=c++11 -fno-builtin-memset jobapplication.cc -o jobapplication -pthread
// []$> g++ -Wall -std=c++11 jobapplication.cc -o jobapplication -pthread
// -std=[c++98, c++11, c++14, c++17, c++20]

#define __STDC_WANT_LIB_EXT1__ 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>

using namespace std;

static std::mutex mtx_cout;

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: acout                                                                                //
//                                                                                             //
// cout based trace output replaced with  acout() << "..."                                     //
// cout in multi thread program, it not print well                                             //
// Asynchronous output                                                                         //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
struct acout
{
  std::unique_lock<std::mutex> lk;
  acout()
    :
    lk(std::unique_lock<std::mutex>(mtx_cout))
  {
    
  }
  
  template<typename T>
  acout& operator<<(const T& _t)
  {
    std::cout << _t;
    return *this;
  }
  
  acout& operator<<(std::ostream& (*fp)(std::ostream&))
  {
    std::cout << fp;
    return *this;
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
//  class: Runnable_c                                                                          //
//                                                                                             //
// Base class for threads                                                                      //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Runnable_c
{
public:
  
  Runnable_c() : m_isStopping(false), m_isStopped(true), m_isRunning(false), m_thread()
  {
    acout() << "Runnable_c::Runnable_c() called" << endl;
  }
  
  virtual ~Runnable_c()
  {
    acout() << "Runnable_c::~Runnable_c() called" << endl;
    
    Join();
  }

  Runnable_c(Runnable_c const&) = delete;
  Runnable_c& operator =(Runnable_c const&) = delete;

  bool WillStop()
  {
    acout() << "Runnable_c::WillStop() called" << endl;
     
    if (!m_isRunning)
      return(false);

    if(m_isStopped)
      return(false);

    if(m_isStopping)
      return(false);
    
    m_isStopping = true;
    
    return(true);
  }

  inline bool IsStopping()
  {
    return m_isStopping;
  }

  inline bool IsStopped()
  {
    return m_isStopped;
  }

  bool Join()
  {
    acout() << "Runnable_c::Join() called: m_isRunning="<< m_isRunning <<" m_isStopping="<<
      m_isStopping << " m_isStopped=" << m_isStopped << endl;
      
    if (!m_isRunning)
      return(false);

    /* if (m_isStopped)
       return; */

    if ((!m_isStopping)&&(!m_isStopped))
    {
      WillStop();
    }
    
    while(!IsStopped()) {}

    
    return (Stop());
  }
  
protected:


  void SetStopped()
  {
    m_isStopping = false;
    m_isStopped = true;
  }

  inline bool IsRunning()
  {
    if((m_isRunning) /* &&(!m_isStopping) */ &&(!m_isStopped))
      return m_isRunning;
    
    return false;
  }

  bool Start()
  {
    acout() << "Runnable_c::Start() called: m_isRunning="<< m_isRunning <<" m_isStopping=" <<
      m_isStopping << " m_isStopped=" << m_isStopped << endl;

    if (m_isRunning)
      return(false);

    if (m_isStopping)
      return(false);

    if (!m_isStopped)
      return(false);
    
    try
    {
      m_thread = std::thread(&Runnable_c::Run, this);
    } catch(...) { }

    m_isRunning = true;
    m_isStopped = false;
    m_isStopping = false;

    return(true);
  }

  virtual void Run() = 0;
  
  bool Stop()
  {
    acout() << "Runnable_c::Stop() called: m_isRunning="<< m_isRunning <<" m_isStopping=" <<
      m_isStopping << " m_isStopped=" << m_isStopped << endl;
    
    if(!m_isRunning)
      return(false);

    if (m_isStopping)
      return(false);

    if (!m_isStopped)
      return(false);
    
 
    acout() << "Thread Join" << endl;
    try
    {
      m_thread.join();
    } catch(...) { }

    m_isRunning = false;

    return(true);
  }
  
private:
  std::atomic<bool> m_isStopping;
  std::atomic<bool> m_isStopped;
  std::atomic<bool> m_isRunning;

  std::thread m_thread;

};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
//  class: MammalBasicFunctions_c                                                              //
//                                                                                             //
// MammalBasicFunctions_c-class carries out basic mammal functions as breathing, sleeping      //
// and eating                                                                                  //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class MammalBasicFunctions_c: public Runnable_c
{
public:
  MammalBasicFunctions_c():m_isEating(false),m_isSleeping(false) 
  {
    EatCounter=0;
    SleepCounter=0;
    
    // Start();
  }
  ~MammalBasicFunctions_c()
  {

    // Join();

  }

  void Start()
  {
    Runnable_c::Start();
  }

  void Stop()
  {
    Runnable_c::Stop();
  }

  inline bool IsEating()
  {
    return m_isEating;
  }

  inline bool IsSleeping()
  {
    return m_isSleeping;
  }
  
protected:

  // virtual void UseLimbsForEat()=0;
  virtual void UseLimbsForEat() {}
  
  void Sleep()
  {
    acout() << "Mammal_c::Sleep() called" << endl;
    
  }
  void Eat()
  {
    acout() << "Mammal_c::Eat() called" << endl;      
    
    UseLimbsForEat();
  }
  void Breath() {}

  virtual void Run()
  {
    while (!IsStopping())
    {

      Breath();
      acout() << "MammalBasicFunction_c::Breath() called" << endl;
      this_thread::sleep_for(chrono::milliseconds(1000));
      
      if((EatCounter++>5)&&(SleepCounter<10)) // not eating when sleeping
      {
	m_isEating=true;
	
	EatCounter=0;
	Eat();
      }
      else
      {
	m_isEating=false;
      }

      if(SleepCounter++>10)
      {
	m_isSleeping=true;
	
	Sleep();

	SleepCounter=(SleepCounter > 20)?0:SleepCounter;
      }
      else
      {
	m_isSleeping=false;
      }
    }

    acout() << "MammalBasicFunction_c::SetStopped() called" << endl;
	
    SetStopped();
  }
private:
  unsigned int EatCounter;
  unsigned int SleepCounter;

  std::atomic<bool> m_isEating;
  std::atomic<bool> m_isSleeping;

};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Mammal_c                                                                             //
//                                                                                             //
// This class carries out basic mammal functions as BeActive()                                 //
//                                                                                             //
// Functions like breathing, sleeping and eating are implemented in another thread and class   //
// MammalBasicFunctions_c                                                                      //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Mammal_c: public Runnable_c
{
public:
  Mammal_c()
  {
  }
  ~Mammal_c()
  {
    // Join();
    // mammalBasicFunctions.Join();
    
  }

  void Start()
  {
    mammalBasicFunctions.Start();
    Runnable_c::Start();
  }

  void Stop()
  {
    Runnable_c::Stop();
    mammalBasicFunctions.Join();
  }

protected:
  virtual void BeActive()
  {
    acout() << "Mammal_c::BeActive() called" << endl;

  }

  // virtual void UseLimbsForEat() {}
  
  virtual void Run()
  {
    while (!IsStopping())
    {
      this_thread::sleep_for(chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    acout() << "Mammal_c::SetStopped() called" << endl;

    SetStopped();
  }

  MammalBasicFunctions_c mammalBasicFunctions;

private:
  
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Primate_c                                                                            //
//                                                                                             //
// primate = kädelliset                                                                        //
// The class implements primate primal basic functions such as BeActive (), UseHands (),       //
// Relax (). Primate even not know how to use hands or work.                                   //
//                                                                                             //
// Functions like breathing, sleeping and eating are implemented in another thread and class   //
// Mammal_c::MammalBasicFunctions_c                                                            //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Primate_c: public Mammal_c
{
public:
  Primate_c()
  {
    Start();
  }
  ~Primate_c()
  {
    Stop();    
  }

protected:

  virtual void UseHands()
  {
    acout() << "Primate_c::UseHands() called" << endl;

  }

  virtual void Relax()
  {
    acout() << "Primate_c::Relax() called" << endl;

  }
  
  virtual void BeActive()
  {
    acout() << "Primate_c::BeActive() called" << endl;

    int randomAction = rand() % 2 +1; //Generates number between 1 - 2

    switch (randomAction)
      {
      case 1:
	//
	UseHands();
	break;
      case 2:
	//
	Relax();
	break;
	
      }
        
  }

  virtual void Run()
  {
    while (!IsStopping())
    {
      this_thread::sleep_for(chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    acout() << "Primate_c::SetStopped() called" << endl;

    SetStopped();
  }
  
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Human_c                                                                              //
//                                                                                             //
// The class carries out basic human functions like BeActive(), UseHands(), Relax(), Speak(),  //
// Work(), Hobby().                                                                            //
//                                                                                             //
// Functions like breathing, sleeping and eating are implemented in another thread and class   //
// Primate_c::Mammal_c::MammalBasicFunctions_c                                                 //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Human_c: public Primate_c
{
public:
  Human_c()
  {
    Start();	
  }
  ~Human_c()
  {
    Stop();
  }
protected:

  virtual void UseHands()
  {
    acout() << "Human_c::UseHands() called" << endl;

  }
  
  virtual void BeActive()
  {
    acout() << "Human_c::BeActive() called" << endl;
	
    int randomAction = rand() % 5 +1; //Generates number between 1 - 5

    switch (randomAction)
      {
      case 1:
	//
	UseHands();

	break;
      case 2:
	//
	Work();
	break;
      case 3:
	//
	Hobby();
	break;
      case 4:
	//
	Speak();
	break;
      case 5:
	//
	Relax();
	break;
	
      }
    
  }

  virtual void Speak()
  {
    acout() << "Human_c::Speak() called" << endl;
    
  }
  
  virtual void Work()
  {
    acout() << "Human_c::Work() called" << endl;	
    
    UseHands();    
  }
  
  virtual void Hobby()
  {
    acout() << "Human_c::Hobby() called" << endl;

  }

  virtual void Relax()
  {
    acout() << "Human_c::Relax() called" << endl;

  }
  
  virtual void Run()
  {
    while (!IsStopping())
    {
      this_thread::sleep_for(chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    acout() << "Human_c::SetStopped() called" << endl;

    SetStopped();
  }

};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: ProjectTask_c                                                                        //
//                                                                                             //
// The class implements task types project manager can send to software developers.            //
//                                                                                             //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class ProjectTask_c
{
public:
  
  enum ProjectTaskType: int
  {
    WRITECODE=1,
    TESTCODE,
    WRITEDOCUMENT,
    ARRANGEMEETING,
    ATTENDMEETING,
    WRITEREPORT,
    VISITCUSTOMER,
    GIVECUSTOMERSUPPORT,
    PUBLISHNEWSOFTWARERELEASE
  };

  static const unsigned int NUMBER_OF_PROJECT_TASK_TYPES;
  
  inline const char* ToString(ProjectTaskType v)
  {
    switch (v)
      {
      case WRITECODE: return "WRITECODE";
      case TESTCODE: return "TESTCODE";
      case WRITEDOCUMENT: return "WRITEDOCUMENT";
      case ARRANGEMEETING: return "ARRANGEMEETING";
      case ATTENDMEETING: return "ATTENDMEETING";
      case WRITEREPORT: return "WRITEREPORT";
      case VISITCUSTOMER: return "VISITCUSTOMER";
      case GIVECUSTOMERSUPPORT: return "GIVECUSTOMERSUPPORT";
      case PUBLISHNEWSOFTWARERELEASE: return "PUBLISHNEWSOFTWARERELEASE";

      default:  return "[Unknown ProjectTaskType]";
      }
  }
  
  ProjectTask_c(unsigned int taskSerialNumber,ProjectTaskType taskType):
    taskSerialNumber(taskSerialNumber),taskType(taskType),
    doneStatus(false), reasonCode(0)
  {
  }
  ~ProjectTask_c () {}

  unsigned int taskSerialNumber;
  ProjectTaskType taskType;
  bool doneStatus;
  int reasonCode;
};

const unsigned int ProjectTask_c::NUMBER_OF_PROJECT_TASK_TYPES=9;

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: template <class T> class ThreadSafeProjectTaskQueue_c                                //
//                                                                                             //
// Theread safe FIFO queue where project manager put the tasks to be carried out by            //
// software developers.                                                                        //
//                                                                                             //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class ThreadSafeProjectTaskQueue_c
{
public:

  ThreadSafeProjectTaskQueue_c() {}
  ~ThreadSafeProjectTaskQueue_c()
  {
    std::lock_guard<std::mutex> lockThis(m_mutex);

    while(!m_queue.empty())
    {
      T m= m_queue.front();
      m_queue.pop();

      if(m==NULL)
      {
	acout() <<
	"ThreadSafeProjectTaskQueue_c::~ThreadSafeProjectTaskQueue_c() FATAL NULL!"
	<< endl;
      }
	
      acout() <<
      "ThreadSafeProjectTaskQueue_c_c::~ThreadSafeProjectTaskQueue_c() deleting taskSerialNumber=" <<
	m->taskSerialNumber << endl;
      
      delete(m); m=NULL;
    }

  }

  bool Empty()
  {
    std::lock_guard<std::mutex> lockThis(m_mutex);

    return (m_queue.empty());
  }
  
  int Size()
  {
    std::lock_guard<std::mutex> lockThis(m_mutex);

    return (m_queue.size());
  }

  void Push(T val)
  {
    std::lock_guard<std::mutex> lockThis(m_mutex);

    m_queue.push (val);
  }

  T Pop(bool &isEmpty)
  {
    std::lock_guard<std::mutex> lockThis(m_mutex);

    if (m_queue.empty())
    {
      isEmpty=true;
      return(NULL);
    }

    isEmpty=false;
    
    T m= m_queue.front();
    m_queue.pop();

    return (m);
  }

  
private:
  std::queue<T> m_queue;
  /* static */ std::mutex m_mutex;

  // m_mutex.lock();
  // m_mutex.unlock();
  
};

ThreadSafeProjectTaskQueue_c <ProjectTask_c *> m_ThreadSafeProjectTaskQueue;

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Agile_c                                                                              //
//                                                                                             //
// This class implements agile methologies used in this software project.                      //
//                                                                                             //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Agile_c
{
public:
  Agile_c() {}
  ~Agile_c() {}

};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Waterfall_c                                                                          //
//                                                                                             //
// This class implements waterfall methologies used in this software project.                  //
//                                                                                             //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Waterfall_c
{
public:
  Waterfall_c() {}
  ~Waterfall_c() {}

};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Cpp_c                                                                                //
//                                                                                             //
// Defines and implements a programming language which is used in a software project.          //
//                                                                                             //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
// Thanks Bjarne Stroustrup, that you created such nice language C++
// https://www.youtube.com/watch?v=JBjjnqG0BP8
class Cpp_c
{
public:
  Cpp_c() {}
  ~Cpp_c() {}
  void WriteCode() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: Java_c                                                                               //
//                                                                                             //
// Defines and implements a programming language which is used in a software project.          //
//                                                                                             //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
class Java_c
{
public:
  Java_c() {}
  ~Java_c() {}
  void WriteCode() {}
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: template <class T> class SoftwareProjectManager_c                                    //
//                                                                                             //
// This class implements project manager functionalities                                       //
//                                                                                             //
// Functions like breathing, sleeping and eating are implemented in another thread and class   //
// Human_c::Primate_c::Mammal_c::MammalBasicFunctions_c                                        //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class SoftwareProjectManager_c: public Human_c
{
public:
  SoftwareProjectManager_c() {}
  ~SoftwareProjectManager_c()
  {
    
    ProjectTask_c *t=NULL;
    bool isEmpty=false;
    
    while((t=m_ThreadSafeProjectTaskQueue.Pop(isEmpty)))
    {
	
      acout() << "SoftwareProjectManager_c <" << GetType() <<
	">::~SoftwareProjectManager_c() deleting taskSerialNumber=" <<
	t->taskSerialNumber << endl;
      
      delete(t); t=NULL;
    }
     
  }

  // void StartProject() {}
  // void StopProject() {}

  // inline bool IsProjectOnGoing() { return true; }


protected:

  virtual void UseHands()
  {
    acout() << "SoftwareProjectManager_c::UseHands() called" << endl;
	
  }
  
  virtual void BeActive()
  {
    acout() << "SoftwareProjectManager_c::BeActive() called" << endl;

    int randomAction = rand() % 10 +1; //Generates number between 1 - 10

    switch (randomAction)
      {
      case 1:
	//
	UseHands();

	break;
      case 2:
	//
	Work();
	break;
      case 3:
	//
	Hobby();
	break;
      case 4:
	//
	Speak();
	break;
      case 5:
	//
	Relax();

	break;
      default:
	// Sometimes Project Manager has to do some job in evening and weekend
	Work();
	break;
      }
    
  }

  virtual void Speak()
  {
    acout() << "SoftwareProjectManager_c::Speak() called" << endl;

  }
  virtual void Work()
  {
    acout() << "SoftwareProjectManager_c::Work() called" << endl;	

    int randomAction = rand() % 12 +1; //Generates number between 1 - 12

    switch (randomAction)
      {
      case 1:
	// Some not so good project managers only are able to shaking hands on air
	ShakingHandsOnAir();
	break;
      case 2:
	WriteReports();
	break;
      case 3:
	ArrangeMeeting();
	break;
      case 4:
	AttendMeeting();
	break;
      case 5:
	VisitCustomer();
	break;
      case 6:
	ManageBudget();
	break;
      default:
	CreateTasksForSWDevelopers();

	break;
      }
  }
  
  virtual void Hobby()
  {
    acout() << "SoftwareProjectManager_c::Hobby() called" << endl;

  }

  virtual void Relax()
  {
    acout() << "SoftwareProjectManager_c::Relax() called" << endl;
    
  }
  
  virtual void Run()
  {
    while (!IsStopping())
    {
      // Project manager has to act faster for multible software developers
      this_thread::sleep_for(chrono::milliseconds(500));
      // this_thread::sleep_for(chrono::milliseconds(1000));

      // If project manager is sleeping or eating, no work done.
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    acout() << "SoftwareProjectManager_c::SetStopped() called" << endl;

    SetStopped();

  }
  
private:
  
  static unsigned int taskSerialNumber;
  
  void ShakingHandsOnAir() {} // heiluttelee käsiä ilmassa

  void WriteReports() {}
  void ArrangeMeeting() {}
  void AttendMeeting() {}
  void VisitCustomer() {}
  void ManageBudget() {}

  // return type of project, agile or waterfall
  static const char* GetType()
  {
    return typeid(T).name();
  }


  void CreateTasksForSWDevelopers()
  {
    // acout() << "SoftwareProjectManager_c <" << GetType() <<
    //  ">::CreateTasksForSWDevelopers() called" << endl;

    // int randomTaskType = rand() % 10 +1; //Generates number between 1 - 10
    int randomTaskType = rand() % ((int)ProjectTask_c::NUMBER_OF_PROJECT_TASK_TYPES) +1;
    //Generate number between [1 - ProjectTask_c::NUMBER_OF_PROJECT_TASK_TYPES]
    
    ProjectTask_c *m_ProjectTask= new ProjectTask_c(taskSerialNumber++,
    (ProjectTask_c::ProjectTaskType)randomTaskType);

    acout() << "SoftwareProjectManager_c <" << GetType() <<
      ">::CreateTasksForSWDevelopers() adding taskSerialNumber=" <<
      m_ProjectTask->taskSerialNumber << " taskType=" <<
      m_ProjectTask->ToString(m_ProjectTask->taskType) << endl;      
    
    m_ThreadSafeProjectTaskQueue.Push(m_ProjectTask);
    
  }
};

// initializing static member. All tasks have serial number for future follow up purpose
// so software developers can report that certain task is done
template <class T>
unsigned int SoftwareProjectManager_c<T>::taskSerialNumber=1;

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// class: template <class T> class SoftwareDeveloper_c                                         //
//                                                                                             //
// This class implements software developer functionalities                                    //
//                                                                                             //
// Functions like breathing, sleeping and eating are implemented in another thread and class   //
// Human_c::Primate_c::Mammal_c::MammalBasicFunctions_c                                        //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class SoftwareDeveloper_c: public Human_c
{
public:
  SoftwareDeveloper_c()
  {

    thisSoftwareDeveloperInstanceNumber=softwareDeveloperInstanceNumber++;
    taskExecutedDuringProject=0;
  }
  
  ~SoftwareDeveloper_c()
  {
    acout() << "SoftwareDeveloper_c <" << GetType() <<
    ">::~SoftwareDeveloper_c() softwareDeveloperInstanceNumber=" <<
    thisSoftwareDeveloperInstanceNumber << " taskExecutedDuringProject=" <<
    taskExecutedDuringProject << endl;      
         
  }

protected:

  virtual void UseHands()
  {
    acout() << "SoftwareDeveloper_c::UseHands() called" << endl;

  }
  
  virtual void BeActive()
  {
    acout() << "SoftwareDeveloper_c::BeActive() called" << endl;

    int randomAction = rand() % 10 +1; //Generates number between 1 - 10

    switch (randomAction)
      {
      case 1:
	//
	UseHands();

	break;
      case 2:
	//
	Work();

	break;
      case 3:
	//
	Hobby();

	break;
      case 4:
	//
	Speak();

	break;
      case 5:
	//
	Relax();

	break;
      default:
	// Sometimes SoftwareDeveloper_c has to do some job in evening and weekend
	Work();

	break;
      }
    
  }

  virtual void Speak()
  {
    acout() << "SoftwareDeveloper_c::Speak() called" << endl;

  }
  virtual void Work()
  {
    acout() << "SoftwareDeveloper_c::Work() called" << endl;	
    
    // int randomAction = rand() % 10 +1; //Generates number between 1 - 10

    // switch (randomAction)
    //   {
    //   case 1:
    //   break;
    //   default:
    ExecuteTasksFromProjectManager();
    //	 break;
    //   }
  }
  
  virtual void Hobby()
  {
    acout() << "SoftwareDeveloper_c::Hobby() called" << endl;

  }

  virtual void Relax()
  {
    acout() << "SoftwareDeveloper_c::Relax() called" << endl;
    
  }
  
  virtual void Run()
  {
    while (!IsStopping())
    {
      this_thread::sleep_for(chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    acout() << "SoftwareDeveloper_c::SetStopped() called" << endl;

    SetStopped();

  }
  
private:

  static unsigned int softwareDeveloperInstanceNumber;
  unsigned int thisSoftwareDeveloperInstanceNumber;

  unsigned int taskExecutedDuringProject;
  
  void WriteCode()
  {
    acout() << "SoftwareDeveloper_c::WriteCode() called" <<
      endl;

  }
  void TestCode()
  {
    acout() << "SoftwareDeveloper_c::TestCode() called" <<
      endl;	

  }
  void WriteDocument()
  {
    acout() << "SoftwareDeveloper_c::WriteDocument() called" <<
      endl;

  }
  void AttendMeeting()
  {
    acout() << "SoftwareDeveloper_c::AttendMeeting() called" <<
      endl;

  }

  void WriteReport()
  {
    acout() << "SoftwareDeveloper_c::WriteReport() called" <<
      endl;

  }
  void ArrangeMeeting()
  {
    acout() << "SoftwareDeveloper_c::ArrangeMeeting() called" <<
      endl;

  }
  void VisitCustomer()
  {
    acout() << "SoftwareDeveloper_c::VisitCustomer() called" <<
      endl;

  }
  void GiveCustomerSupport()
  {
    acout() << "SoftwareDeveloper_c::GiveCustomerSupport() called" <<
      endl;

  }
  void PublishNewSoftwareRelease()
  {
    acout() << "SoftwareDeveloper_c::PublishNewSoftwareRelease() called" <<
      endl;

  }

  // return type of project, agile or waterfall
  static const char* GetType()
  {
    return typeid(T).name();
  }

  void ExecuteTasksFromProjectManager()
  {
    // acout() << "SoftwareDeveloper_c <" << GetType() <<
    //  ">::ExecuteTasksFromProjectManager() called" << endl;

    bool isEmpty=true;
    ProjectTask_c *m_ProjectTask=m_ThreadSafeProjectTaskQueue.Pop(isEmpty);

    if(m_ProjectTask)
    {
      taskExecutedDuringProject++;
      
      acout() << "SoftwareDeveloper_c[" <<
	thisSoftwareDeveloperInstanceNumber <<
	"] <" << GetType() <<
	">::ExecuteTasksFromProjectManager() retrieving taskSerialNumber=" <<
	m_ProjectTask->taskSerialNumber << " taskType=" <<
	m_ProjectTask->ToString(m_ProjectTask->taskType) << endl;      

      switch (m_ProjectTask->taskType)
      {
      case ProjectTask_c::WRITECODE:
	//
	WriteCode();
	break;
      case ProjectTask_c::TESTCODE:
	//
	TestCode();
	break;
      case ProjectTask_c::WRITEDOCUMENT:
	//
	WriteDocument();
	break;
      case ProjectTask_c::ARRANGEMEETING:
	//
	ArrangeMeeting();
	break;
      case ProjectTask_c::ATTENDMEETING:
	//
	AttendMeeting();
	break;
      case ProjectTask_c::WRITEREPORT:
	//
	WriteReport();
	break;
      case ProjectTask_c::VISITCUSTOMER:
	//
	VisitCustomer();
	break;
      case ProjectTask_c::GIVECUSTOMERSUPPORT:
	//
	GiveCustomerSupport();
	break;
      case ProjectTask_c::PUBLISHNEWSOFTWARERELEASE:
	//
	PublishNewSoftwareRelease();
	break;
	
      default:
	// If ProjectTask type is unknown, not defined.
	//
	acout() << "Unknown ProjectTaskType" <<
	  endl;	
	break;
      }

      delete(m_ProjectTask); m_ProjectTask=NULL;

    }
    
  }
};

template <class T>
unsigned int SoftwareDeveloper_c<T>::softwareDeveloperInstanceNumber=1;

#ifndef __STDC_LIB_EXT1__
/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// function: secure_memset                                                                     //
//                                                                                             //
// This function is safe to use because it won't be optimized out by compiler.                 //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
void *secure_memset (unsigned char *v,unsigned char c,size_t n)
{
  // The trick is to use 'volatile' keyword and then compiler stop
  // to optimize it out
  volatile unsigned char *p = v;
  while (n--) *p++ = c;
  return v; 
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// function: main                                                                              //
//                                                                                             //
// Standard main function                                                                      //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{

  /* initialize random seed: */
  // srand (time(NULL));
  srand(static_cast <unsigned int> (time(0)));

  unsigned int n = thread::hardware_concurrency();
  cout << n << " concurrent threads are supported by CPU." << endl;

  std::hash <std::string> hash;
  string password;

  // hash of password
  unsigned long hashedPassword = 6072375419398818283;
  // correct password is "password"
  
  cout << "Give Password to Execute Program:";
  cin >> password;

  unsigned long hashedPasswordGuess = hash(password);

  if (hashedPasswordGuess  == hashedPassword)
  {
    cout << "Password is correct!" << endl;
  }
  else
  {
    cout << "Password is wrong!" << endl;
    // cout << "Hash:" << hashedPasswordGuess <<endl;
    exit(-1);
  }

  hashedPassword=hashedPasswordGuess=0;


#ifdef __STDC_LIB_EXT1__
  set_constraint_handler_s(ignore_handler_s);

  // As standard memset(...) might be optimized out by compiled
  // memset_s is guaranteed that it won't be optimized out
  // Unluckily it is not supported by Fedora 27 Linux
  memset_s(password.c_str(),password.length(),0,password.length());
  acout() << "memset_s(...) Secured!" << endl;
#else
  // standard C memset might be optimized out by compiler
  // memset((void *)password.c_str(),0,password.length());
  secure_memset((unsigned char *)password.c_str(),0,password.length());
  acout() << "secure_memset(...) Secured!" << endl;
#endif

  // std::fill_n could be used too
  // std::fill_n((volatile char*)p, n*sizeof(T), 0);
  
  password.clear();

  
  /*
  // Code section for testing Mammal_c
  Mammal_c mammal;
  mammal.Start();
  this_thread::sleep_for(chrono::milliseconds(1000*30));
  acout() << "Stopping!" << endl;
  mammal.WillStop();
  while(!mammal.IsStopped()) {}
  acout() << "Stopped!" << endl;
  // mammal.Stop();
  acout() << "Completed!" << endl;
  */

  /*
  // Code section for testing Primate_c
  Primate_c primate;
  primate.Start();
  this_thread::sleep_for(chrono::milliseconds(1000*30));
  // mammal.Stop(); ??
  primate.WillStop();
  while(!primate.IsStopped()) {}
  acout() << "Stopped!" << endl;
  // primate.Stop();
  acout() << "Completed!" << endl;
  */

  /*
  // Code section for testing Human_c
  Human_c human;
  human.Start();
  this_thread::sleep_for(chrono::milliseconds(1000*30));
  human.WillStop();
  while(!human.IsStopped()) {}
  acout() << "Stopped!" << endl;
  // Human.Stop();
  acout() << "Completed!" << endl;
  */

  /*
  // Code section for testing SoftwareProjectManager_c
  SoftwareProjectManager_c <Agile_c> mainProjectManager;
  mainProjectManager.Start();
  this_thread::sleep_for(chrono::milliseconds(1000*60));
  mainProjectManager.WillStop();
  while(!mainProjectManager.IsStopped()) {}
  acout() << "Stopped!" << endl;
  // mainProjectManager.Stop();
  acout() << "Completed!" << endl;
  */

  /*
  // Code section for testing minimal SoftwareProject
  SoftwareProjectManager_c <Agile_c> mainProjectManager;
  SoftwareDeveloper_c <Agile_c> softwareDeveloper;
  
  mainProjectManager.Start();
  softwareDeveloper.Start();
  
  // project last 1 minutes
  this_thread::sleep_for(chrono::milliseconds(1000*60));
  
  mainProjectManager.WillStop();
  softwareDeveloper.WillStop();
  
  while(!mainProjectManager.IsStopped()) {}
  while(!softwareDeveloper.IsStopped()) {}
  
  acout() << "Stopped!" << endl;
  // mainProjectManager.Stop();
  // softwareDeveloper.Stop();
  acout() << "Completed!" << endl;

  */


  // /*
  // Code section for testing group of 12 software developers in the SoftwareProject
  const int number_of_software_developers_in_project=12;
  
  SoftwareProjectManager_c <Agile_c> mainProjectManager;
  SoftwareDeveloper_c <Agile_c> softwareDeveloper[number_of_software_developers_in_project];

  mainProjectManager.Start();
  for (auto i=0; i<number_of_software_developers_in_project; i++)
    softwareDeveloper[i].Start();

  // project last 1 minutes
  this_thread::sleep_for(chrono::milliseconds(1000*60));
  
  mainProjectManager.WillStop();
  for (auto i=0; i<number_of_software_developers_in_project; i++)
    softwareDeveloper[i].WillStop();
  
  while(!mainProjectManager.IsStopped()) {}
  for (auto i=0; i<number_of_software_developers_in_project; i++)
  {  
    while(!softwareDeveloper[i].IsStopped()) {}
  }
  
  acout() << "Stopped!" << endl;
  // mainProjectManager.Stop();
  // softwareDeveloper.Stop();
  acout() << "Completed!" << endl;

  // */

  return 0;
}

// TODO
// SoftwareDeveloper_c could send back status information about executed
// ProjectTasks to ProjectManager_c, maybe second FIFO.
// Utilization of templates i.e., Java_c,Cpp_c and
// Agile_c, Waterfall_c is now 'light'. Due to lack of time.
