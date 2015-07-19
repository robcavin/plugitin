using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;
using System;

public class WindowsWindowsManager : MonoBehaviour {


	[DllImport ("WindowsWindowsPlugin")]
	private static extern void SayHello();

	[DllImport ("WindowsWindowsPlugin")]
	private static extern void SetRenderMatrix(float[] matrix);

	[DllImport ("WindowsWindowsPlugin")]
	private static extern void SetViewport(float[] viewport);

	// Use this for initialization
	void Start () {
		SayHello ();
		Debug.Log ("yey");
	}
	
	// Update is called once per frame
	void Update () {

	}

	void OnPostRender() {

		Matrix4x4 CameraToEye = Camera.main.projectionMatrix;
		Matrix4x4 WorldToCamera = Camera.main.worldToCameraMatrix;

		Vector3 pos = new Vector3(0,0,-1);
		Quaternion orientation  = new Quaternion(0,0,0,1);
		Vector3 scale = new Vector3(1,1,1);
		Matrix4x4 ObjectToWorld = Matrix4x4.TRS(pos,orientation,scale);

		Matrix4x4 ObjectToEye = ObjectToWorld * WorldToCamera * CameraToEye;
		float[] matrixFloatArray = {
			ObjectToEye.m00,
			ObjectToEye.m01,
			ObjectToEye.m02,
			ObjectToEye.m03,
			ObjectToEye.m10,
			ObjectToEye.m11,
			ObjectToEye.m12,
			ObjectToEye.m13,
			ObjectToEye.m20,
			ObjectToEye.m21,
			ObjectToEye.m22,
			ObjectToEye.m23,
			ObjectToEye.m30,
			ObjectToEye.m31,
			ObjectToEye.m32,
			ObjectToEye.m33};

		//SetRenderMatrix (matrixFloatArray);

		float[] viewport = {Camera.main.pixelRect.xMin,Camera.main.pixelRect.yMin, Camera.main.pixelRect.width, Camera.main.pixelRect.height};
		//SetViewport (viewport);

		GL.IssuePluginEvent (0);
	}
}
