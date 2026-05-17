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
//  eating, sleeping. So, while sleeping or eating not do any work. The first thread           //
//  carries out the actual work(payload).                                                      //
//                                                                                             //
//  (Total project manager and 12 software developers mean 26 threads((12*2)+2)).              //
//                                                                                             //
//                                                                                             //
//  In the below are instructions for compiling the program in the target system.              //
//  This program is coded on Fedora 27-44 Linux-system, but it most probably can be compiled   //
//  and executed on ,e.g., MacOS, Windows and Raspberry Pi. It is standard STL program and it  //
//  follows C++20 standard.                                                                    //
//                                                                                             //
//  Created by Markku Mikkanen on 30/04/2018.                                                  //
//  Updated by Markku Mikkanen on 12/05/2026                                                   //                      //
//  Copyright © 2018 Markku Mikkanen. All rights reserved.                                     //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

// Compile instructions:
// []$> g++ -Wall -std=c++20 -fno-builtin-memset jobapplication.cc -o jobapplication -pthread
// []$> g++ -Wall -std=c++20 jobapplication.cc -o jobapplication -pthread
// -std=[c++98, c++11, c++14, c++17, c++20, c++23]

// or:
// []$> cmake -S . -B build
// []$> cmake --build build
// []$> ./build/jobapplication

// ONko vielä pätevä?
// #define __STDC_WANT_LIB_EXT1__ 1

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
#include <vector>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <random>
#include <stop_token> // C++20

// Moderni säieturvallinen tulostus
template<typename... Args>
void safe_print(Args&&... args) {
    static std::mutex cout_mtx;
    std::lock_guard lock(cout_mtx);
    (std::cout << ... << args) << std::endl;
}

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
  
  Runnable_c() : m_isStopping(false), m_isStopped(true), m_isRunning(false) // , m_thread()
  {
    safe_print("Runnable_c::Runnable_c() called");
  }
  
  virtual ~Runnable_c()
  {
    safe_print("Runnable_c::~Runnable_c() called");
    // Join();
  }

  Runnable_c(Runnable_c const&) = delete;
  Runnable_c& operator =(Runnable_c const&) = delete;

  bool WillStop()
  {
    safe_print("Runnable_c::WillStop() called");
     
    if (!m_isRunning || m_isStopped || m_isStopping) return false;

    m_isStopping = true;
    return true;

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
    safe_print("Runnable_c::Join() called: m_isRunning=", m_isRunning, " m_isStopping=", m_isStopping, " m_isStopped=", m_isStopped);
      
    if (!m_isRunning)
      return(false);

    if ((!m_isStopping)&&(!m_isStopped))
    {
      WillStop();
    }
    
    std::unique_lock<std::mutex> lock(m_stopMutex);
    m_stopCondition.wait(lock, [this] { return m_isStopped.load(); });

    if (m_thread.joinable()) m_thread.join();
    m_isRunning = false;
    return true;
    
  }
  
protected:


  void SetStopped()
  {
    m_isStopping = false;
    m_isStopped = true;
    m_stopCondition.notify_all(); // Ilmoitetaan odottajille (Join)
  }

  inline bool IsRunning()
  {
    if((m_isRunning) /* &&(!m_isStopping) */ &&(!m_isStopped))
      return m_isRunning;
    
    return false;
  }

  bool Start()
  {
    safe_print("Runnable_c::Start() called: m_isRunning=", m_isRunning, " m_isStopping=", m_isStopping, " m_isStopped=", m_isStopped);

    if (m_isRunning || m_isStopping) return false;

    try
    {
      m_thread = std::jthread([this](std::stop_token stoken) {
            this->m_stopToken = stoken; // Tallennetaan jäsenmuuttujaan, jos halutaan käyttää muualla luokassa
            this->Run(stoken);
        });

      // m_thread = std::jthread(&Runnable_c::Run, this);
    } catch(...) { }

    m_isRunning = true;
    m_isStopped = false;
    m_isStopping = false;

    return(true);
  }

  // virtual void Run() = 0;

  // Päivitetty Run-metodi ottaa vastaan stop_tokenin
  virtual void Run(std::stop_token stoken) = 0;
  
  bool Stop()
  {
    safe_print("Runnable_c::Stop() called: m_isRunning=", m_isRunning, " m_isStopping=", m_isStopping, " m_isStopped=", m_isStopped);

    if((!m_isRunning)||(!m_isStopped)||(m_isStopping))
      return(false);

    safe_print("Thread Join");
    try
    {
      m_thread.join();
    } catch(...) { }

    m_isRunning = false;

    return(true);
  }
  
  // Palauttaa viittauksen atomiseen muuttujaan, jotta PopWait voi tarkkailla sitä
  std::atomic<bool>& GetStoppingAtomic() {
    return m_isStopping;
  }

  std::stop_token m_stopToken; // Tallennetaan token tänne, jotta muut metodit voivat käyttää sitä tarvittaessa

private:
  std::atomic<bool> m_isStopping;
  std::atomic<bool> m_isStopped;
  std::atomic<bool> m_isRunning;

  std::jthread m_thread;

  std::mutex m_stopMutex;
  std::condition_variable m_stopCondition;

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
    safe_print("Mammal_c::Sleep() called");
  }
  void Eat()
  {
    safe_print("Mammal_c::Eat() called");
    
    UseLimbsForEat();
  }
  void Breath() {}

  virtual void Run(std::stop_token stoken) override
  {
    while (!stoken.stop_requested())
    {

      Breath();
      safe_print("MammalBasicFunction_c::Breath() called");
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      
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

    safe_print("MammalBasicFunction_c::SetStopped() called");
	
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
    safe_print("Mammal_c::BeActive() called");

  }

  // virtual void UseLimbsForEat() {}
  
  virtual void Run(std::stop_token stoken) override
  {
    while (!stoken.stop_requested())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    safe_print("Mammal_c::SetStopped() called");

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
private:

  // Alustetaan heti määrittelyssä, niin ne ovat käyttövalmiita
  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist{0, 1};

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
    safe_print("Primate_c::UseHands() called");

  }

  virtual void Relax()
  {
    safe_print("Primate_c::Relax() called");

  }
  
  virtual void BeActive()
  {
    safe_print("Primate_c::BeActive() called");

    int randomAction = dist(rng); //Generates number between 1 - 2

    switch (randomAction)
      {
        case 0:
        UseHands();
        break;

        case 1:
        Relax();
        break;
	
      }
        
  }

  virtual void Run(std::stop_token stoken) override
  {
    while (!stoken.stop_requested())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    safe_print("Primate_c::SetStopped() called");

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
private:
// Alustetaan heti määrittelyssä, niin ne ovat käyttövalmiita
  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist{0, 4};
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
    safe_print("Human_c::UseHands() called");

  }
  
  virtual void BeActive()
  {
    safe_print("Human_c::BeActive() called");
	
    int randomAction =  dist(rng); //Generates number between 0 - 4

    switch (randomAction)
      {
        case 0:
        UseHands();
        break;

        case 1:
        Work();
        break;

        case 2:
        Hobby();
        break;

        case 3:
        Speak();
        break;

        case 4:
        Relax();
        break;

        default:
          safe_print("Human_c::BeActive() called with unknown action");
          break;

      }
    
  }

  virtual void Speak()
  {
    safe_print("Human_c::Speak() called");
  }
  
  virtual void Work()
  {
    safe_print("Human_c::Work() called");
    UseHands();
  }
  
  virtual void Hobby()
  {
    safe_print("Human_c::Hobby() called");

  }

  virtual void Relax()
  {
    safe_print("Human_c::Relax() called");

  }
  
  virtual void Run(std::stop_token stoken) override
  {
    while (!stoken.stop_requested())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    safe_print("Human_c::SetStopped() called");

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
    WRITECODE=0,
    TESTCODE,
    WRITEDOCUMENT,
    ARRANGEMEETING,
    ATTENDMEETING,
    WRITEREPORT,
    VISITCUSTOMER,
    GIVECUSTOMERSUPPORT,
    PUBLISHNEWSOFTWARERELEASE,
    COUNT_NUMBEROFTASKS
  };

  // static const unsigned int NUMBER_OF_PROJECT_TASK_TYPES;
  
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

// const unsigned int ProjectTask_c::NUMBER_OF_PROJECT_TASK_TYPES=9;

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
      T m= std::move(m_queue.front());
      m_queue.pop();

      if(m)
      {
        safe_print("ThreadSafeProjectTaskQueue_c_c::~ThreadSafeProjectTaskQueue_c() deleting taskSerialNumber=" 
          , m->taskSerialNumber);
      }
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

    m_queue.push (std::move(val));

    m_cond.notify_one(); // Herätetään yksi odottava kehittäjä

  }

  T Pop(bool &isEmpty)
  {
    std::lock_guard<std::mutex> lockThis(m_mutex);

    if (m_queue.empty())
    {
      isEmpty=true;
      return T();
    }

    isEmpty=false;
    
    T m= std::move(m_queue.front());
    m_queue.pop();

    return (m);
  }

  // ThreadSafeProjectTaskQueue_c sisällä
  T PopWait(std::stop_token stoken) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // Odottaa kunnes tehtävä tulee TAI säiettä pyydetään pysähtymään
    m_cond.wait(lock, [this, &stoken] { 
        return !m_queue.empty() || stoken.stop_requested(); 
    });

    if (m_queue.empty()) return T();
  
    T m = std::move(m_queue.front());
    m_queue.pop();
    return m;
  }

  // Lisää notify_all sulkemista varten
  void WakeAll() { m_cond.notify_all(); }

private:
  std::queue<T> m_queue;
  /* static */ std::mutex m_mutex;

  std::condition_variable m_cond;

  // m_mutex.lock();
  // m_mutex.unlock();
  
};

ThreadSafeProjectTaskQueue_c<std::unique_ptr<ProjectTask_c>> m_ThreadSafeProjectTaskQueue;
// ThreadSafeProjectTaskQueue_c <ProjectTask_c *> m_ThreadSafeProjectTaskQueue;

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
private:
// Alustetaan heti määrittelyssä, niin ne ovat käyttövalmiita
  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist{0, 9};


public:
  SoftwareProjectManager_c() {

  }
  ~SoftwareProjectManager_c()
  {
    
    // ProjectTask_c *t=NULL;
    bool isEmpty=false;
    
    while(auto t=m_ThreadSafeProjectTaskQueue.Pop(isEmpty))
    {
	
      safe_print("SoftwareProjectManager_c <", GetType(),
	    ">::~SoftwareProjectManager_c() deleting taskSerialNumber=",
	    t->taskSerialNumber);
      
      // delete(t); t=NULL;
    }
     
  }

  // void StartProject() {}
  // void StopProject() {}

  // inline bool IsProjectOnGoing() { return true; }


protected:

  virtual void UseHands()
  {
    safe_print("SoftwareProjectManager_c::UseHands() called");

  }
  
  virtual void BeActive()
  {
    safe_print("SoftwareProjectManager_c::BeActive() called");

    int randomAction =  dist(rng); //Generates number between 1 - 10

    switch (randomAction)
      {
        case 0:
        UseHands();
        break;

        case 1:
        Work();
        break;

        case 2:
        Hobby();
        break;

        case 3:
        Speak();
        break;

        case 4:
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
    safe_print("SoftwareProjectManager_c::Speak() called");

  }
  virtual void Work()
  {
    safe_print("SoftwareProjectManager_c::Work() called");

    int randomAction = dist(rng); //Generates number between 1 - 12

    switch (randomAction)
      {
        case 0:
        // Some not so good project managers only are able to shaking hands on air
        ShakingHandsOnAir();
        break;

        case 1:
        WriteReports();
        break;

        case 2:
        ArrangeMeeting();
        break;

        case 3:
        AttendMeeting();
        break;

        case 4:
        VisitCustomer();
        break;

        case 5:
        ManageBudget();
        break;

        default:
        CreateTasksForSWDevelopers();

        break;
      }
  }
  
  virtual void Hobby()
  {
    safe_print("SoftwareProjectManager_c::Hobby() called");

  }

  virtual void Relax()
  {
    safe_print("SoftwareProjectManager_c::Relax() called");
    
  }
  
  virtual void Run(std::stop_token stoken) override
  {
    while (!stoken.stop_requested())
    {
      // Project manager has to act faster for multible software developers
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
      // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      // If project manager is sleeping or eating, no work done.
      if(!mammalBasicFunctions.IsSleeping()||(!mammalBasicFunctions.IsEating()))
      {
	BeActive();
      }

    }

    mammalBasicFunctions.WillStop();
    while(!mammalBasicFunctions.IsStopped()) {}

    safe_print("SoftwareProjectManager_c::SetStopped() called");

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

    // Projektipäällikkö (Main-säie tässä esimerkissä) luo tehtäviä
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, (int)ProjectTask_c::ProjectTaskType::COUNT_NUMBEROFTASKS-1);

    int randomTaskType = dist(rng);
    // int randomTaskType = rand() % ((int)ProjectTask_c::ProjectTaskType::COUNT_NUMBEROFTASKS) +1;
    //Generate number between [1 - ProjectTask_c::NUMBER_OF_PROJECT_TASK_TYPES]
    
    auto m_ProjectTask= std::make_unique<ProjectTask_c>(taskSerialNumber++,
    static_cast<ProjectTask_c::ProjectTaskType>(randomTaskType));

    safe_print("SoftwareProjectManager_c <", GetType(),
      ">::CreateTasksForSWDevelopers() adding taskSerialNumber=",
      m_ProjectTask->taskSerialNumber , " taskType=" ,
      m_ProjectTask->ToString(m_ProjectTask->taskType));
    
    m_ThreadSafeProjectTaskQueue.Push(std::move(m_ProjectTask));
    
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
private:
// Alustetaan heti määrittelyssä, niin ne ovat käyttövalmiita
  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist{0, 9};

public:
  SoftwareDeveloper_c()
  {

    thisSoftwareDeveloperInstanceNumber=softwareDeveloperInstanceNumber++;
    taskExecutedDuringProject=0;
  }
  
  ~SoftwareDeveloper_c()
  {
    safe_print("SoftwareDeveloper_c <", GetType(),
      ">::~SoftwareDeveloper_c() softwareDeveloperInstanceNumber=",
      thisSoftwareDeveloperInstanceNumber , " taskExecutedDuringProject=" ,
      taskExecutedDuringProject);
         
  }

protected:

  virtual void UseHands()
  {
    safe_print("SoftwareDeveloper_c::UseHands() called");

  }
  
  virtual void BeActive()
  {
    safe_print("SoftwareDeveloper_c::BeActive() called");

    int randomAction = dist(rng); //Generates number between 1 - 10

    switch (randomAction)
      {
      case 0:
	//
	UseHands();

	break;
      case 1:
	//
	Work();

	break;
      case 2:
	//
	Hobby();

	break;
      case 3:
	//
	Speak();

	break;
      case 4:
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
    safe_print("SoftwareDeveloper_c::Speak() called");

  }
  virtual void Work() override
  {
    safe_print("SoftwareDeveloper_c::Work() called");	
    
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
    safe_print("SoftwareDeveloper_c::Hobby() called");

  }

  virtual void Relax()
  {
    safe_print("SoftwareDeveloper_c::Relax() called");
  }
  
virtual void Run(std::stop_token stoken) override {
  while (!stoken.stop_requested()) {
    // Jos kehittäjä ei nuku tai syö, hän yrittää tehdä töitä
    if (!mammalBasicFunctions.IsSleeping() && !mammalBasicFunctions.IsEating()) {
        BeActive(); 
    } else {
        // Jos on tauolla, pidetään pieni uni ettei loop pyöri liian lujaa
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  mammalBasicFunctions.WillStop();

  // jthread hoitaa pysäytyksen, mutta kutsutaan silti nämä siisteyden vuoksi
  mammalBasicFunctions.Stop(); // jthreadilla Stop() on nyt turvallinen
  SetStopped();

  safe_print("SoftwareDeveloper_c::SetStopped() called");
  SetStopped();
}

private:

  static unsigned int softwareDeveloperInstanceNumber;
  unsigned int thisSoftwareDeveloperInstanceNumber;

  unsigned int taskExecutedDuringProject;
  
  void WriteCode()
  {
    safe_print("SoftwareDeveloper_c::WriteCode() called");

  }
  void TestCode()
  {
    safe_print("SoftwareDeveloper_c::TestCode() called");

  }
  void WriteDocument()
  {
    safe_print("SoftwareDeveloper_c::WriteDocument() called");

  }
  void AttendMeeting()
  {
    safe_print("SoftwareDeveloper_c::AttendMeeting() called");

  }

  void WriteReport()
  {
    safe_print("SoftwareDeveloper_c::WriteReport() called");

  }
  void ArrangeMeeting()
  {
    safe_print("SoftwareDeveloper_c::ArrangeMeeting() called");

  }
  void VisitCustomer()
  {
    safe_print("SoftwareDeveloper_c::VisitCustomer() called");

  }
  void GiveCustomerSupport()
  {
    safe_print("SoftwareDeveloper_c::GiveCustomerSupport() called");

  }
  void PublishNewSoftwareRelease()
  {
    safe_print("SoftwareDeveloper_c::PublishNewSoftwareRelease() called");

  }

  // return type of project, agile or waterfall
  static const char* GetType()
  {
    return typeid(T).name();
  }

//   void ExecuteTasksFromProjectManager()
void ExecuteTasksFromProjectManager(){
        // Tämä kutsu nyt BLOCKAA (pysäyttää säikeen) kunnes tehtävä tulee
        // välittäen tiedon IsStopping() tilasta
        auto m_ProjectTask = m_ThreadSafeProjectTaskQueue.PopWait(m_stopToken);

        if (m_ProjectTask) {
            taskExecutedDuringProject++;
            safe_print("SoftwareDeveloper_c[", thisSoftwareDeveloperInstanceNumber, 
                       "] suorittaa tehtävää: ", m_ProjectTask->ToString(m_ProjectTask->taskType));
            
            // Simuloidaan työn kestoa, ettei kaikki tapahdu silmänräpäyksessä
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Suoritetaan tehtävä (switch-lause säilyy ennallaan...)
        }


      switch (m_ProjectTask->taskType)
      {
        case ProjectTask_c::WRITECODE:
        WriteCode();
        break;

        case ProjectTask_c::TESTCODE:
	      TestCode();
	      break;

        case ProjectTask_c::WRITEDOCUMENT:
	      WriteDocument();
        break;

        case ProjectTask_c::ARRANGEMEETING:
      	ArrangeMeeting();
	      break;

        case ProjectTask_c::ATTENDMEETING:
        AttendMeeting();
        break;

        case ProjectTask_c::WRITEREPORT:
        WriteReport();
        break;

        case ProjectTask_c::VISITCUSTOMER:
        VisitCustomer();
        break;

        case ProjectTask_c::GIVECUSTOMERSUPPORT:
        GiveCustomerSupport();
        break;

        case ProjectTask_c::PUBLISHNEWSOFTWARERELEASE:
        PublishNewSoftwareRelease();
        break;

        default:
        // If ProjectTask type is unknown, not defined.
        //
        safe_print("Unknown ProjectTaskType");
        break;
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
// int main (int argc, char *argv[])
int main ()
{

  unsigned int n = std::thread::hardware_concurrency();
  safe_print(n ," concurrent threads are supported by CPU.");

  std::hash <std::string> hash;
  std::string password;

  // hash of password
  unsigned long hashedPassword = 6072375419398818283;
  // correct password is "password"
  
  safe_print("Give Password to Execute Program:");
  std::cin >> password;

  unsigned long hashedPasswordGuess = hash(password);

  if (hashedPasswordGuess  == hashedPassword)
  {
    safe_print("Password is correct!");
  }
  else
  {
    safe_print("Password is wrong!");
    // safe_print("Hash:" << hashedPasswordGuess <<endl);
    secure_memset((unsigned char *)password.data(), 0, password.length());
    password.clear();
    exit(-1);
  }

  hashedPassword=hashedPasswordGuess=0;


  secure_memset((unsigned char *)password.data(),0,password.length());
  safe_print("secure_memset(...) Secured!");

  // std::fill_n could be used too
  // std::fill_n((volatile char*)p, n*sizeof(T), 0);
  
  password.clear();


  // Code section for testing group of 12 software developers in the SoftwareProject
  const int number_of_software_developers_in_project=12;
  
  SoftwareProjectManager_c <Agile_c> mainProjectManager;
  SoftwareDeveloper_c <Agile_c> softwareDeveloper[number_of_software_developers_in_project];

  mainProjectManager.Start();
  for (auto i=0; i<number_of_software_developers_in_project; i++)
    softwareDeveloper[i].Start();

  // project last 60 minutes
  std::this_thread::sleep_for(std::chrono::milliseconds(1000*60));
  
  mainProjectManager.WillStop();
  for (auto i=0; i<number_of_software_developers_in_project; i++)
    softwareDeveloper[i].WillStop();
  
  // HERÄTETÄÄN JONO: Tämä vapauttaa PopWait-metodissa jumissa olevat säikeet
  m_ThreadSafeProjectTaskQueue.WakeAll();

  // while(!mainProjectManager.IsStopped()) {}
  mainProjectManager.Join();

  for (auto i=0; i<number_of_software_developers_in_project; i++)
  {  
  //   while(!softwareDeveloper[i].IsStopped()) {}
    softwareDeveloper[i].Join();
  }
  
  safe_print("Stopped!");
  // mainProjectManager.Stop();
  // softwareDeveloper.Stop();
  safe_print("Completed!");

  // */

  return 0;
}

// TODO
// SoftwareDeveloper_c could send back status information about executed
// ProjectTasks to ProjectManager_c, maybe second FIFO.
// Utilization of templates i.e., Java_c,Cpp_c and
// Agile_c, Waterfall_c is now 'light'. Due to lack of time.
