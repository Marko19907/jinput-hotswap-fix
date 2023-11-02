/*
 * %W% %E%
 *
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#include "rawwinver.h"
#include <windows.h>
#include <jni.h>
#include "net_java_games_input_RawInputEventQueue.h"
#include "util.h"

extern jmethodID addMouseEvent_method;
extern jmethodID addKeyboardEvent_method;

JNIEXPORT void JNICALL Java_net_java_games_input_RawInputEventQueue_nRegisterDevices(JNIEnv *env, jclass unused, jint flags, jlong hwnd_addr, jobjectArray device_infos) {
	BOOL res;
	jclass device_info_class;
	jmethodID getUsage_method;
	jmethodID getUsagePage_method;
	RAWINPUTDEVICE *devices;
	RAWINPUTDEVICE *device;
	jsize num_devices = (*env)->GetArrayLength(env, device_infos);
	USHORT usage;
	USHORT usage_page;
	int i;
	HWND hwnd = (HWND)(INT_PTR)hwnd_addr;

/*	res = GetRegisteredRawInputDevices(NULL, &num_devices, sizeof(RAWINPUTDEVICE));
	if (num_devices > 0) {
		devices = (RAWINPUTDEVICE *)malloc(num_devices*sizeof(RAWINPUTDEVICE));
		res = GetRegisteredRawInputDevices(devices, &num_devices, sizeof(RAWINPUTDEVICE));
		if (res == -1) {
			throwIOException(env, "Failed to get registered raw devices (%d)\n", GetLastError());
			return;
		}
		for (i = 0; i < num_devices; i++) {
			printfJava(env, "from windows: registered: %d %d %p (of %d)\n", devices[i].usUsagePage, devices[i].usUsage, devices[i].hwndTarget, num_devices);
		}
		free(devices);
	}*/
	device_info_class = (*env)->FindClass(env, "net/java/games/input/RawDeviceInfo");
	if (device_info_class == NULL)
		return;
	getUsage_method = (*env)->GetMethodID(env, device_info_class, "getUsage", "()I");
	if (getUsage_method == NULL)
		return;
	getUsagePage_method = (*env)->GetMethodID(env, device_info_class, "getUsagePage", "()I");
	if (getUsagePage_method == NULL)
		return;
	devices = (RAWINPUTDEVICE *)malloc(num_devices*sizeof(RAWINPUTDEVICE));
	if (devices == NULL) {
		throwIOException(env, "Failed to allocate device structs\n");
		return;
	}
	for (i = 0; i < num_devices; i++) {
		jobject device_obj = (*env)->GetObjectArrayElement(env, device_infos, i);
		if (device_obj == NULL) {
			free(devices);
			return;
		}
		usage = (*env)->CallIntMethod(env, device_obj, getUsage_method);
		if ((*env)->ExceptionOccurred(env)) {
			free(devices);
			return;
		}
		usage_page = (*env)->CallIntMethod(env, device_obj, getUsagePage_method);
		if ((*env)->ExceptionOccurred(env)) {
			free(devices);
			return;
		}
		device = devices + i;
		device->usUsagePage = usage_page;
		device->usUsage = usage;
		device->dwFlags = flags;
		device->hwndTarget = hwnd;
	}
	res = RegisterRawInputDevices(devices, num_devices, sizeof(RAWINPUTDEVICE));
	free(devices);
	if (!res)
		throwIOException(env, "Failed to register raw devices (%d)\n", GetLastError());
}

JNIEXPORT void JNICALL Java_net_java_games_input_RawInputEventQueue_nPostMessage(JNIEnv *env, jobject self, jlong hwnd_handle) {
    HWND hwnd = (HWND)(INT_PTR)hwnd_handle;
    PostMessage((HWND) hwnd, WM_USER, 0, 0);
}

JNIEXPORT void JNICALL Java_net_java_games_input_RawInputEventQueue_nPoll(JNIEnv *env, jobject self, jlong hwnd_handle) {
	MSG msg;
	HWND hwnd = (HWND)(INT_PTR)hwnd_handle;
	jmethodID addMouseEvent_method;
	jmethodID addKeyboardEvent_method;
	UINT input_size;
	RAWINPUT *input_data;
	LONG time;
	jclass self_class = (*env)->GetObjectClass(env, self);

	jclass cls = (*env)->GetObjectClass(env, self);

    addMouseEvent_method = (*env)->GetStaticMethodID(env, cls, "addMouseEvent", "(JJIIIJJJJ)V");
    addKeyboardEvent_method = (*env)->GetStaticMethodID(env, cls, "addKeyboardEvent", "(JJIIIIJ)V");

	if (self_class == NULL)
		return;
	if (addMouseEvent_method == NULL)
		return;
	if (addKeyboardEvent_method == NULL)
		return;
}
