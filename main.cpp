#define WINVER 0x0602
#define _WIN32_WINNT 0x0602
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>
#include "Windows.h"
#include "io.h"
#include "fcntl.h"
#include "ShaiG.h"

#pragma comment(lib, "advapi32.lib")

namespace chTime = std::chrono;
typedef chTime::high_resolution_clock chTimeHRC;

//////////////////////////////////USER DEFINED TYPES///////////////////////////////////////////
class logFile{
    std::ofstream file;
    bool open;
    
    public:
    logFile():file{},open{0}{}
    logFile(std::wstring fileName):file{ fileName,std::ios::trunc},open{1}{}
    logFile(std::wstring fileName,bool dummy):file{ fileName,std::ios::app},open{1}{}
    ~logFile(){file<<'\n'<<'\n';file.close();}
    
    void OPEN(std::wstring filename){
        file.open(filename,std::ios::trunc);open = 1;
    }
    void OPEN(std::wstring filename, bool dummy){
        file.open(filename,std::ios::app);open=1;
    }

    template<typename Twritable>
    bool write(Twritable txt,bool newLine=1){
        if(open){
            std::string dateTime;
            ShaiG::getDateTimeStr(dateTime);
            if(newLine)file<<dateTime<<"          "<<txt<<'\n';
            else file<<dateTime<<"          "<<txt;
            return 1;
        }
        else return 0;
    }
};
//////////////////////////////////USER DEFINED TYPES///////////////////////////////////////////

//////////////////////////////////FUNCTIONS///////////////////////////////////////////
bool strcmpW(wchar_t* strA, wchar_t* strB);

template <typename Tchar>
void addStrToBuff(Tchar* buff, Tchar* str){
    unsigned endIndex{0};
	while(buff[endIndex]!='\0')++endIndex;
    for(unsigned c{0};str[c]!='\0';++c){
        buff[endIndex] = str[c]; ++endIndex;
    }
}

template <typename Tchar>
void remEndPath(Tchar* str){
	unsigned endIndex{0};
	while(str[endIndex]!='\0')++endIndex;
	while(str[endIndex]!='\\'){
		str[endIndex] = '\0';
		--endIndex;
	}
}
//////////////////////////////////FUNCTIONS///////////////////////////////////////////


//////////////////////////////////Windows Service Global Definitions/variables and Function Declarations///////////////////////////////////////////
#define SERVICENAME L"svcTest"
wchar_t svcPath[MAX_PATH];
SERVICE_STATUS_HANDLE statusHandle;
SERVICE_STATUS svcStatusStruct;
HANDLE svcStopEvent{INVALID_HANDLE_VALUE};

void svcInstall(logFile& iLog, std::wstring& svcPath);
void svcUnInstall();
void WINAPI svcMainW(DWORD argc, LPWSTR* argv);
void WINAPI svcCtrlHandler(DWORD dwCtrl);
void startWork();//void workerThread(); 

//////////////////////////////////Windows Service Global Definitions/variables and Function Declarations///////////////////////////////////////////
logFile dLog;
int wmain(int argc, wchar_t* argv[]){
    if (argc>1){
        wchar_t installStr[]{L"/install"};
        wchar_t unInstallStr[]{L"/delete"};
        if (strcmpW(argv[1],installStr)){
            std::wstring svcPath(MAX_PATH,L'\0');
            GetModuleFileNameW( NULL, svcPath.data(), MAX_PATH );
            std::wstring svcProcPath{svcPath};
            remEndPath(svcPath.data());wchar_t ilFileName[]{L"InstallLog.txt"};
            addStrToBuff(svcPath.data(),ilFileName);
            logFile iLog{svcPath,1};
            //set console input and output to unicode
            _setmode(_fileno(stdin), _O_U16TEXT);

            //disable console quick edit mode so mouse click won't pause the process
            DWORD consoleMode;
            HANDLE inputHandle{ GetStdHandle(STD_INPUT_HANDLE) };
            GetConsoleMode(inputHandle, &consoleMode);
            SetConsoleMode(inputHandle, consoleMode & (~ENABLE_QUICK_EDIT_MODE));
            svcInstall(iLog, svcProcPath);return 0;
        }else if(strcmpW(argv[1],unInstallStr)){
            //set console input and output to unicode
            _setmode(_fileno(stdin), _O_U16TEXT);

            //disable console quick edit mode so mouse click won't pause the process
            DWORD consoleMode;
            HANDLE inputHandle{ GetStdHandle(STD_INPUT_HANDLE) };
            GetConsoleMode(inputHandle, &consoleMode);
            SetConsoleMode(inputHandle, consoleMode & (~ENABLE_QUICK_EDIT_MODE));
            svcUnInstall();return 0;
        }
        else {
            std::wstring svcPath(MAX_PATH,L'\0');
            GetModuleFileNameW( NULL, svcPath.data(), MAX_PATH );
            remEndPath(svcPath.data());wchar_t lFileName[]{L"Log.txt"};
            addStrToBuff(svcPath.data(),lFileName);
            dLog.OPEN(svcPath,1);    
            dLog.write("Service was run with unapproved argument!");
            return 0;
        } 
    }
    std::wstring svcPath(MAX_PATH,L'\0');
    GetModuleFileNameW( NULL, svcPath.data(), MAX_PATH );
    remEndPath(svcPath.data());wchar_t lFileName[]{L"Log.txt"};
    addStrToBuff(svcPath.data(),lFileName);
    dLog.OPEN(svcPath,1);    
    SERVICE_TABLE_ENTRYW serviceTable[]{
        {const_cast<LPWSTR>(SERVICENAME),static_cast<LPSERVICE_MAIN_FUNCTIONW>(svcMainW)},
        {NULL,NULL}
    };

    if(StartServiceCtrlDispatcherW(serviceTable)==0){
        dLog.write("StartServiceCtrlDipatcher Failed!");
        return 0;
    }else dLog.write("Test Starting Service");
    dLog.write("Service Stoppped");
    return 0;
}

void WINAPI svcMainW(DWORD argc, LPWSTR* argv){
    statusHandle = RegisterServiceCtrlHandlerW(SERVICENAME,svcCtrlHandler);
    if (statusHandle==0)return;
    ZeroMemory(&svcStatusStruct,sizeof(svcStatusStruct));
    svcStatusStruct.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    svcStatusStruct.dwServiceSpecificExitCode = 0;
    svcStatusStruct.dwCurrentState = SERVICE_START_PENDING;
    svcStatusStruct.dwWin32ExitCode = 0;
    svcStatusStruct.dwCheckPoint = 0;

    if(SetServiceStatus(statusHandle,&svcStatusStruct)==0)return;
    
    svcStopEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
    if(svcStopEvent==NULL) {
        svcStatusStruct.dwControlsAccepted = 0;
        svcStatusStruct.dwCurrentState = SERVICE_STOPPED;
        svcStatusStruct.dwWin32ExitCode = GetLastError();
        svcStatusStruct.dwCheckPoint = 1;
        SetServiceStatus(statusHandle,&svcStatusStruct);
        return;
    }
    
    svcStatusStruct.dwCurrentState = SERVICE_RUNNING;
    svcStatusStruct.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    svcStatusStruct.dwWin32ExitCode = 0;
    svcStatusStruct.dwCheckPoint = 0;

    SetServiceStatus(statusHandle,&svcStatusStruct);
    
    dLog.write("Service Started Successfully");

    startWork(); //call main worker function
    
    CloseHandle(svcStopEvent);

    svcStatusStruct.dwControlsAccepted = 0;
    svcStatusStruct.dwCurrentState = SERVICE_STOPPED;
    svcStatusStruct.dwWin32ExitCode = 0;
    svcStatusStruct.dwCheckPoint = 3;
    dLog.write("Service Stopping");
    SetServiceStatus(statusHandle,&svcStatusStruct);

    return;
}

void WINAPI svcCtrlHandler(DWORD dwCtrl){
     // Handle the requested control code. 
    switch(dwCtrl) {  
      case SERVICE_CONTROL_STOP: 
        if(svcStatusStruct.dwCurrentState != SERVICE_RUNNING) break;
        svcStatusStruct.dwControlsAccepted = 0;
        svcStatusStruct.dwCurrentState = SERVICE_STOP_PENDING;
        svcStatusStruct.dwWin32ExitCode = 0;
        svcStatusStruct.dwCheckPoint = 4; 
        SetEvent(svcStopEvent);break;
      
      default: 
         break;
   }
   return; 
}

void startWork(){
    dLog.write("Worker Function Started");
    std::ofstream svcTest{"c:\\users\\user\\desktop\\svcTest\\checking.txt",std::ios::trunc};
    
    chTime::minutes twoMinutes{2};
    chTime::time_point<chTimeHRC> newTime;
    chTime::time_point<chTimeHRC> baseTime{
        chTimeHRC::now()
    };

    while(WaitForSingleObject(svcStopEvent,0)!=WAIT_OBJECT_0){
        if(ShaiG::durationPassed(baseTime,newTime,twoMinutes)){
            baseTime = chTimeHRC::now();
            svcTest<<"checking\n";
        }
    }
    svcTest.close();
    dLog.write("Worker Function Returning");
    return;
}

bool strcmpW(wchar_t* strA, wchar_t* strB){
    for(unsigned c{0};1;++c){
        if (strA[c]!=strB[c])return 0;
        if (strA[c] == L'\0')return 1;
    }
}

void svcInstall(logFile& iLog,std::wstring& svcPath){
    std::cout<<"Installing Service\n";
    iLog.write("Installing clientMS service");
    
    //wchar_t szPath[MAX_PATH];//buffer to store full service path
    //copies process name to szPath buffer
    //GetModuleFileNameW( NULL, szPath, MAX_PATH );

    // Get a handle to the SCM database. 
    SC_HANDLE schSCManager {
        OpenSCManagerW( 
            NULL,                         // local computer
            SERVICES_ACTIVE_DATABASEW,    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS         // full access rights
        )
    };   
 
    if (schSCManager == NULL){
        dLog.write("Failed to connect to SCM database");
        return;
    };

    // Create the service
    SC_HANDLE schService {
        CreateServiceW( 
            schSCManager,              // SCM database 
            SERVICENAME,               // name of service 
            L"Service Test",               // service name to display 
            SERVICE_ALL_ACCESS,        // desired access 
            SERVICE_WIN32_OWN_PROCESS, // service type 
            SERVICE_AUTO_START,        // start type 
            SERVICE_ERROR_NORMAL,      // error control type 
            svcPath.data(),            // path to service's binary 
            NULL,                      // no load ordering group 
            NULL,                      // no tag identifier 
            NULL,                      // no dependencies 
            NULL,                      // LocalSystem account 
            NULL                       // no password 
        )
    };                     
 
    if (schService == NULL){
        iLog.write("Create service failed");
        iLog.write("Installation Failed");
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
    }
    else{
        std::wcout<<"Service, "<<SERVICENAME<<", installed successfully\n";
        iLog.write("Create Service Successfull");
        iLog.write("Installation Successfull");
        _SERVICE_DESCRIPTIONW svcDescriptStruct;
        wchar_t svcDescript[]{
            L"This is our test service. It tests for a working service."
        };
        svcDescriptStruct.lpDescription = svcDescript;
        if(ChangeServiceConfig2W(schService,1,&svcDescriptStruct)==0){
            iLog.write("Set service description Failed");
        }else{
            iLog.write("Set service description Successfull");
        }

        StartServiceW(schService,NULL,NULL);
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
    }
}

void svcUnInstall(){
    // Get a handle to the SCM database. 
    SC_HANDLE schSCManager {
        OpenSCManagerW( 
            NULL,                         // local computer
            SERVICES_ACTIVE_DATABASEW,    // ServicesActive database 
            SC_MANAGER_ALL_ACCESS         // full access rights
        )
    };

    if (schSCManager == NULL){
        std::wcout<<L"Delete Sevice Failed\n";
        dLog.write("Failed to connect to SCM database");
        return;
    };

    SC_HANDLE schSvc {
        OpenServiceW(
            schSCManager,
            SERVICENAME,
            SERVICE_ALL_ACCESS
        )
    };

    if (schSvc == NULL){
        std::wcout<<L"Delete Sevice Failed\n";
        dLog.write("Open Service Failed");
        CloseServiceHandle(schSvc); 
        CloseServiceHandle(schSCManager);
        return;
    }

    if(DeleteService(schSvc)==0){
        std::wcout<<L"Delete Sevice Failed\n";
        dLog.write("Delete Service Failed");
        CloseServiceHandle(schSvc); 
        CloseServiceHandle(schSCManager);
    }else{
        std::wcout<<L"Deleted Service, "<<SERVICENAME
        <<L", Successfully\n";
        CloseServiceHandle(schSvc); 
        CloseServiceHandle(schSCManager);
    }
}

/* void workerThread(){
    std::ofstream svcTest{"c:\\Program Files\\svcTest\\checking.txt",std::ios::trunc};
    
    chTime::minutes twoMinutes{2};
    chTime::time_point<chTimeHRC> newTime;
    chTime::time_point<chTimeHRC> baseTime{
        chTimeHRC::now()
    };

    while(WaitForSingleObject(svcStopEvent,0)!=WAIT_OBJECT_0){
        if(ShaiG::durationPassed(baseTime,newTime,twoMinutes)){
            baseTime = chTimeHRC::now();
            svcTest<<"checking\n";
        }
    }
    svcTest.close();
    return;
} */