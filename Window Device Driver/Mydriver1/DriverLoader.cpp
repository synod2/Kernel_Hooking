#include <stdio.h>
#include <Windows.h>


int InstallDriver(wchar_t *driverPath, wchar_t *driverName);
int StartDriver(wchar_t *driverName);
int StopDriver(wchar_t* driverName);
int UnInstallDriver(wchar_t* driverName);


int main(int argc,char * argv[]) {
	char* mode=NULL;

	if (argc > 0) {
		mode = argv[1];
	}
	
	if (!strcmp(mode,"start")){
	
		printf("\ninstall driver\n");
		InstallDriver((wchar_t*)L"mydriver\\MyDriver1.sys", (wchar_t*)L"mydriver");
		StartDriver((wchar_t*)L"mydriver");
	}
	else if (!strcmp(mode,"stop"))
	{
		printf("\nremove driver\n");
		StopDriver((wchar_t*)L"mydriver");
		UnInstallDriver((wchar_t*)L"mydriver");
	}
	else {
		printf("usage : start / stop \n");
	}
	
}

/*----------------------------
	드라이버 설치 함수
----------------------------*/
//1. SCM 생성 및 핸들 오픈 
//2. 서비스 생성 
//3. 각종 예외처리 및 핸들 정리
int InstallDriver(wchar_t* driverPath, wchar_t* driverName) {
	//SCM 핸들,서비스 핸들 선언
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	//SCM 핸들 오픈 
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	
	//핸들 유효성 검사
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//서비스 생성 - HKEY_LOCAL_MACHINE\SYSTEM\CurrnetControlSet\Services\DriverName
	hService = CreateServiceW(hSCManager, driverName, driverName,
		GENERIC_READ,		//서비스 필요 권한 
		SERVICE_KERNEL_DRIVER,	//서비스 종류
		SERVICE_DEMAND_START,	//서비스 시작 유형 
		SERVICE_ERROR_NORMAL,	//서비스 오류 처리 유형
		driverPath,
		NULL,					//로그 규칙 그룹
		NULL,					//태그 ID 
		NULL,					//의존성
		NULL,					//서비스 시작 이름
		NULL					//비밀번호
	);

	//핸들 유효성 검사 
	if (!hService) {
		printf("CreateService Error \n");
		return 1;
	}

	//완료 핸들 close

	if (hService) {
		CloseServiceHandle(hService);
		hService = NULL;
	}
	
	if (hSCManager) {
		CloseServiceHandle(hSCManager);
		hSCManager = NULL;
	}
	return 0;
}

/*------------------------
	드라이버 서비스 시작 함수 
-------------------------*/
//1. SCM생성 및 핸들 오픈
//2. 서비스 오픈
//3. 서비스 시작 
//4. 각종 예외처리 및 핸들 정리 

int StartDriver(wchar_t* driverName) {

	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//서비스 오픈 
	hService = OpenService(hSCManager, driverName, SERVICE_START);
	if(!hService)
	{
		printf("Service Open Error \n");
		return 1;
	}
	
	//서비스 시작
	if(!StartService(hService, 0, NULL))
	{
		printf("Service Start Error \n");
		return 1;
	}

	//완료 핸들 close
	if (hService) {
		CloseServiceHandle(hService);
		hService = NULL;
	}

	if (hSCManager) {
		CloseServiceHandle(hSCManager);
		hSCManager = NULL;
	}
	return 0;
}

/*------------------------
	서비스 중단 함수
-------------------------*/
//1. SCM생성 및 핸들 오픈
//2. 서비스 오픈 - 중단 
//3. 각종 예외처리 및 핸들 정리 
int StopDriver(wchar_t* driverName) {
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//서비스 오픈 - 서비스 중단 옵션 
	hService = OpenService(hSCManager, driverName, SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (!hService)
	{
		printf("Service Open Error \n");
		return 1;
	}
	//서비스 중단 제어코드 전송 
	SERVICE_STATUS st;
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &st)) {
		printf("Service Stop Error \n");
	}
	
	//완료 핸들 close
	if (hService) {
		CloseServiceHandle(hService);
		hService = NULL;
	}

	if (hSCManager) {
		CloseServiceHandle(hSCManager);
		hSCManager = NULL;
	}
	return 0;
}

/*------------------------
	서비스 삭제 함수
-------------------------*/
//1. SCM생성 및 핸들 오픈
//2. 서비스 오픈 
//3. 서비스 삭제
//4. 각종 예외처리 및 핸들 정리 
int UnInstallDriver(wchar_t* driverName) {
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//서비스 오픈 - 서비스 삭제 옵션 
	hService = OpenService(hSCManager, driverName, DELETE);
	if (!hService)
	{
		printf("Service Open Error \n");
		return 1;
	}
	 
	//서비스 삭제
	if (!DeleteService(hService)) {
		printf("Service delete Error \n");
	}

	//완료 핸들 close
	if (hService) {
		CloseServiceHandle(hService);
		hService = NULL;
	}

	if (hSCManager) {
		CloseServiceHandle(hSCManager);
		hSCManager = NULL;
	}
	return 0;
};

