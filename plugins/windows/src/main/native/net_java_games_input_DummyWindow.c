/*
 * %W% %E%
 *
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#include <windows.h>
#include <jni.h>
#include "net_java_games_input_DummyWindow.h"
#include "util.h"

static const TCHAR* DUMMY_WINDOW_NAME = "JInputControllerWindow";

// Global reference to the JVM
JavaVM* jvm;

// Global vars
jmethodID addMouseEvent_method;
jmethodID addKeyboardEvent_method;

static void handleMouseEvent(JNIEnv *env, jmethodID add_method, LONG time, RAWINPUT *data) {
    jclass cls = (*env)->FindClass(env, "net/java/games/input/RawInputEventQueue");
    if (cls == NULL) {
        return;  // Class not found
    }
    (*env)->CallStaticVoidMethod(env, cls, add_method,
        (jlong)(INT_PTR)data->header.hDevice,
        (jlong)time,
        (jint)data->data.mouse.usFlags,
        (jint)data->data.mouse.usButtonFlags,
        (jint)(SHORT)data->data.mouse.usButtonData,
        (jlong)data->data.mouse.ulRawButtons,
        (jlong)data->data.mouse.lLastX,
        (jlong)data->data.mouse.lLastY,
        (jlong)data->data.mouse.ulExtraInformation
    );
}

static void handleKeyboardEvent(JNIEnv *env, jmethodID add_method, LONG time, RAWINPUT *data) {
    jclass cls = (*env)->FindClass(env, "net/java/games/input/RawInputEventQueue");
    if (cls == NULL) {
        return;  // Class not found
    }
    (*env)->CallStaticVoidMethod(env, cls, add_method,
        (jlong)(INT_PTR)data->header.hDevice,
        (jlong)time,
        (jint)data->data.keyboard.MakeCode,
        (jint)data->data.keyboard.Flags,
        (jint)data->data.keyboard.VKey,
        (jint)data->data.keyboard.Message,
        (jlong)data->data.keyboard.ExtraInformation
    );
}

static LRESULT CALLBACK DummyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    JNIEnv* env;
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_6;
    (*jvm)->AttachCurrentThread(jvm, (void**)&env, &args);

	switch (uMsg) {
	    case WM_INPUT:
	        // Handle WM_INPUT message.
	        UINT input_size;
	        RAWINPUT *input_data;
	        LONG time;

	        time = GetMessageTime();
	        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &input_size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
	            throwIOException(env, "Failed to get raw input data size (%d)\n", GetLastError());
	            return DefWindowProc(hwnd, uMsg, wParam, lParam);
	        }
	        input_data = (RAWINPUT *)malloc(input_size);
	        if (input_data == NULL) {
	            throwIOException(env, "Failed to allocate input data buffer\n");
	            return DefWindowProc(hwnd, uMsg, wParam, lParam);
	        }
	        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, input_data, &input_size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
	            free(input_data);
	            throwIOException(env, "Failed to get raw input data (%d)\n", GetLastError());
	            return DefWindowProc(hwnd, uMsg, wParam, lParam);
	        }
	        switch (input_data->header.dwType) {
	            case RIM_TYPEMOUSE:
	                handleMouseEvent(env, addMouseEvent_method, time, input_data);
	                break;
	            case RIM_TYPEKEYBOARD:
	                handleKeyboardEvent(env, addKeyboardEvent_method, time, input_data);
	                break;
	            default:
	                /* ignore other types of message */
	                break;
	        }
	        free(input_data);
	        break;

	    case WM_USER:
	        // Dummy message to wake up the message loop, do nothing
	        break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static BOOL RegisterDummyWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)DummyWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)NULL;
	wcex.lpszClassName	= DUMMY_WINDOW_NAME;
	wcex.hIconSm		= NULL;
	return RegisterClassEx(&wcex);
}

// The VM calls JNI_OnLoad when the native library is loaded
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT jlong JNICALL Java_net_java_games_input_DummyWindow_createWindow(JNIEnv *env, jclass unused) {
    HINSTANCE hInst = GetModuleHandle(NULL);
	HWND hwndDummy;
	WNDCLASSEX class_info;
	class_info.cbSize = sizeof(WNDCLASSEX);
	class_info.cbClsExtra = 0;
	class_info.cbWndExtra = 0;

	if (!GetClassInfoEx(hInst, DUMMY_WINDOW_NAME, &class_info)) {
		// Register the dummy input window
		if (!RegisterDummyWindow(hInst)) {
			throwIOException(env, "Failed to register window class (%d)\n", GetLastError());
			return 0;
		}
	}

    // Create the dummy input window
    hwndDummy = CreateWindow(DUMMY_WINDOW_NAME, NULL,
        WS_POPUP | WS_ICONIC,
        0, 0, 0, 0, NULL, NULL, hInst, NULL);
    if (hwndDummy == NULL) {
		throwIOException(env, "Failed to create window (%d)\n", GetLastError());
        return 0;
    }
	return (jlong)(intptr_t)hwndDummy;
}

JNIEXPORT void JNICALL Java_net_java_games_input_DummyWindow_nDestroy(JNIEnv *env, jclass unused, jlong hwnd_address) {
	HWND hwndDummy = (HWND)(INT_PTR)hwnd_address;
	MSG msg;

    // Makes sure that the window has no more messages before it gets killed
	while (PeekMessage(&msg, hwndDummy, 0, 0, PM_REMOVE)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}

	BOOL result = DestroyWindow(hwndDummy);
	if (!result) {
		throwIOException(env, "Failed to destroy window (%d)\n", GetLastError());
	}
}
