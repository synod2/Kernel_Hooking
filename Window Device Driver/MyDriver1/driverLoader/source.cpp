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
	����̹� ��ġ �Լ�
----------------------------*/
//1. SCM ���� �� �ڵ� ���� 
//2. ���� ���� 
//3. ���� ����ó�� �� �ڵ� ����
int InstallDriver(wchar_t* driverPath, wchar_t* driverName) {
	//SCM �ڵ�,���� �ڵ� ����
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	//SCM �ڵ� ���� 
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	
	//�ڵ� ��ȿ�� �˻�
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//���� ���� - HKEY_LOCAL_MACHINE\SYSTEM\CurrnetControlSet\Services\DriverName
	hService = CreateServiceW(hSCManager, driverName, driverName,
		GENERIC_READ,		//���� �ʿ� ���� 
		SERVICE_KERNEL_DRIVER,	//���� ����
		SERVICE_DEMAND_START,	//���� ���� ���� 
		SERVICE_ERROR_NORMAL,	//���� ���� ó�� ����
		driverPath,
		NULL,					//�α� ��Ģ �׷�
		NULL,					//�±� ID 
		NULL,					//������
		NULL,					//���� ���� �̸�
		NULL					//��й�ȣ
	);

	//�ڵ� ��ȿ�� �˻� 
	if (!hService) {
		printf("CreateService Error \n");
		return 1;
	}

	//�Ϸ� �ڵ� close

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
	����̹� ���� ���� �Լ� 
-------------------------*/
//1. SCM���� �� �ڵ� ����
//2. ���� ����
//3. ���� ���� 
//4. ���� ����ó�� �� �ڵ� ���� 

int StartDriver(wchar_t* driverName) {

	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//���� ���� 
	hService = OpenService(hSCManager, driverName, SERVICE_START);
	if(!hService)
	{
		printf("Service Open Error \n");
		return 1;
	}
	
	//���� ����
	if(!StartService(hService, 0, NULL))
	{
		printf("Service Start Error \n");
		return 1;
	}

	//�Ϸ� �ڵ� close
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
	���� �ߴ� �Լ�
-------------------------*/
//1. SCM���� �� �ڵ� ����
//2. ���� ���� - �ߴ� 
//3. ���� ����ó�� �� �ڵ� ���� 
int StopDriver(wchar_t* driverName) {
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//���� ���� - ���� �ߴ� �ɼ� 
	hService = OpenService(hSCManager, driverName, SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (!hService)
	{
		printf("Service Open Error \n");
		return 1;
	}
	//���� �ߴ� �����ڵ� ���� 
	SERVICE_STATUS st;
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &st)) {
		printf("Service Stop Error \n");
	}
	
	//�Ϸ� �ڵ� close
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
	���� ���� �Լ�
-------------------------*/
//1. SCM���� �� �ڵ� ����
//2. ���� ���� 
//3. ���� ����
//4. ���� ����ó�� �� �ڵ� ���� 
int UnInstallDriver(wchar_t* driverName) {
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
	{
		printf("SCManager Open Error \n");
		return 1;
	}

	//���� ���� - ���� ���� �ɼ� 
	hService = OpenService(hSCManager, driverName, DELETE);
	if (!hService)
	{
		printf("Service Open Error \n");
		return 1;
	}
	 
	//���� ����
	if (!DeleteService(hService)) {
		printf("Service delete Error \n");
	}

	//�Ϸ� �ڵ� close
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

