#include <windows.h>

LRESULT CALLBACK WindowFunc(HWND,UINT,WPARAM, LPARAM);

char szName[] = "Hallo";

int _CRT_fmode = 0;
int _CRT_glob = 0;

int __main(int argc, char **argv)
{
	return 0;
}


int main(int argc, char **argv)
{
	HWND hwnd;
	HMENU hmenu;
	MSG msg;
	WNDCLASSEX wc1;
	HINSTANCE hInst = 0;
	int nWinMode = SW_SHOW;

	RECT rect;

	wc1.hInstance = hInst;
	wc1.lpszClassName = szName;
	wc1.lpfnWndProc = WindowFunc;
	wc1.style = 0;
	wc1.cbSize = sizeof(WNDCLASSEX);
	wc1.hIcon = NULL;
	wc1.hIconSm = NULL;

	wc1.hCursor = NULL;
	wc1.lpszMenuName = NULL;

	wc1.cbClsExtra = 0;
	wc1.cbWndExtra = 0;

	wc1.hbrBackground = NULL;

	if ( !RegisterClassEx(&wc1)) return 0;

	hmenu = CreateMenu();

	hwnd = CreateWindowEx(0, szName, "test", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
		NULL,hmenu,hInst, NULL);

      
	ShowWindow(hwnd,nWinMode);
	UpdateWindow(hwnd);

	SetWindowText(hwnd,"Hallo");

	DrawMenuBar(hwnd);	
//	SendMessage( hwnd, WM_MOVE, 0,MAKELONG(0,0));
//        SendMessage( hwnd, WM_PAINT, GetWindowDC(hwnd),0);
	while(GetMessage(&msg,NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Sleep(10000);
	return msg.wParam;
}


	//printf("hallo\n");
	
	

LRESULT CALLBACK WindowFunc(HWND hwnd,UINT message,WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
