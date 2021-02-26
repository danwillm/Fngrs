#include "openvr_driver.h"
#include "bones.h"
#include "driverlog.h"
#include <thread>
#include <atomic>
#include <chrono>

#include "serial_port.h"
#include "quat_utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>

#define DEVICE_NAME "fngrs"

static const char* c_settings_section = "driver_fngrs";
static const char* device_manufacturer = "danwillm";
static const char* device_controller_type = DEVICE_NAME;
static const char* device_model_number = DEVICE_NAME "1";
static const char* right_controller_serial = "FNGRS1";
static const char* left_controller_serial = "FNGRS2";
static const char* device_render_model_name = "{" DEVICE_NAME "}/rendermodels/" DEVICE_NAME;
static const char* device_input_profile_path = "{" DEVICE_NAME "}/input/" DEVICE_NAME "_profile.json";

//Indexes for components - anything that you can bind in the steamvr bindings dashboard
static const enum ComponentIndex : int {
	JOYSTICK_X = 0,
	JOYSTICK_Y = 1,
	JOYSTICK_BTN = 2,
	BTN_TRIGGER = 3,
	BTN_A = 4
};
/*
	Main class for fngrs
*/
class Fngrs : public vr::ITrackedDeviceServerDriver
{
public:
	uint32_t m_id;
	vr::DriverPose_t m_pose;
	vr::VRInputComponentHandle_t m_skeleton;
	vr::ETrackedControllerRole m_role;
	vr::HmdVector3_t m_offset_vector;

	std::atomic<bool> m_active;

	std::thread m_skeletal_thread;
	std::thread m_pose_thread;

	bool m_found_controller;
	int m_controller_id;

	Serial* m_arduino_serial;
	char m_arduino_port[1024];

	std::atomic<bool> m_found_arduino;

	int m_last_component_value[6] = { 0,0,0,0,0,0 };
	vr::VRInputComponentHandle_t m_h_component_values[8];

	int m_frame_count;

	Fngrs()
		: m_found_controller(false),
		m_active(false),
		m_found_arduino(false),
		m_frame_count(0)
	{}
	/*
		Initialise a controller
	*/
	void Init(vr::ETrackedControllerRole role)
	{
		m_role = role;
		DebugDriverLog("This role is %i", role);
	}
	vr::EVRInitError Activate(uint32_t unObjectId) override
	{
		m_id = unObjectId;
		m_pose = { 0 };
		m_pose.poseIsValid = true;
		m_pose.result = vr::TrackingResult_Running_OK;
		m_pose.deviceIsConnected = true;
		m_pose.qDriverFromHeadRotation.w = 1;
		m_pose.qWorldFromDriverRotation.w = 1;
		const char* serial_number = m_role == vr::TrackedControllerRole_RightHand ? "FNGRS1" : "FNGRS2";

		vr::PropertyContainerHandle_t props = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_id);
		vr::VRProperties()->SetStringProperty(props, vr::Prop_SerialNumber_String, serial_number);
		vr::VRProperties()->SetStringProperty(props, vr::Prop_ModelNumber_String, device_model_number);
		//VRProperties()->SetStringProperty(props, Prop_RenderModelName_String, device_render_model_name);
		vr::VRProperties()->SetStringProperty(props, vr::Prop_ManufacturerName_String, device_manufacturer);
		vr::VRProperties()->SetInt32Property(props, vr::Prop_ControllerRoleHint_Int32, m_role);
		vr::VRProperties()->SetInt32Property(props, vr::Prop_DeviceClass_Int32, (int32_t)vr::TrackedDeviceClass_Controller);
		vr::VRProperties()->SetInt32Property(props, vr::Prop_ControllerHandSelectionPriority_Int32, (int32_t)100000);
		vr::VRProperties()->SetStringProperty(props, vr::Prop_InputProfilePath_String, device_input_profile_path);
		vr::VRProperties()->SetStringProperty(props, vr::Prop_ControllerType_String, device_controller_type);

		//Setup buttons and joysticks
		vr::VRDriverInput()->CreateScalarComponent(props, "/input/joystick/x", &m_h_component_values[ComponentIndex::JOYSTICK_X], vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
		vr::VRDriverInput()->CreateScalarComponent(props, "/input/joystick/y", &m_h_component_values[ComponentIndex::JOYSTICK_Y], vr::VRScalarType_Absolute, vr::VRScalarUnits_NormalizedTwoSided);
		vr::VRDriverInput()->CreateBooleanComponent(props, "/input/joystick/click", &m_h_component_values[ComponentIndex::JOYSTICK_BTN]);
		vr::VRDriverInput()->CreateBooleanComponent(props, "/input/A/click", &m_h_component_values[ComponentIndex::BTN_A]);

		//TODO: this should be a scalar component, so that users can set their own grip poses, but for now, it's a boolean, as we need to figure out the best way for deciding the value of each finger.
		vr::VRDriverInput()->CreateBooleanComponent(props, "/input/grip/click", &m_h_component_values[6]);

		//This haptic component doesn't do anything (at least, yet), but some games require it to be bound to something, so just create a component for them to use.
		vr::VRDriverInput()->CreateHapticComponent(props, "output/haptic", &m_h_component_values[7]);


		float c_x_offset = vr::VRSettings()->GetFloat(c_settings_section, "x_offset");
		float c_y_offset = vr::VRSettings()->GetFloat(c_settings_section, "y_offset");
		float c_z_offset = vr::VRSettings()->GetFloat(c_settings_section, "z_offset");

		m_pose.poseTimeOffset = vr::VRSettings()->GetFloat(c_settings_section, "pose_offset");

		if (m_role == vr::TrackedControllerRole_RightHand) {
			vr::VRDriverInput()->CreateBooleanComponent(props, "/input/trigger/click", &m_h_component_values[ComponentIndex::BTN_TRIGGER]);

			m_offset_vector = { c_x_offset, c_y_offset, c_z_offset };

			vr::EVRInputError error = vr::VRDriverInput()->CreateSkeletonComponent(props,
				"/input/skeleton/right",
				"/skeleton/hand/right",
				"/pose/raw",
				vr::VRSkeletalTracking_Partial,
				right_fist_pose,
				NUM_BONES,
				&m_skeleton);

			vr::VRSettings()->GetString(c_settings_section, "right_arduino_port", m_arduino_port, sizeof(m_arduino_port));

		}
		else {
			vr::VRDriverInput()->CreateBooleanComponent(props, "/input/system/click", &m_h_component_values[ComponentIndex::BTN_TRIGGER]);

			m_offset_vector = { -c_x_offset, c_y_offset, c_z_offset };

			vr::EVRInputError error = vr::VRDriverInput()->CreateSkeletonComponent(props,
				"/input/skeleton/left",
				"/skeleton/hand/left",
				"/pose/raw",
				vr::VRSkeletalTracking_Partial,
				left_fist_pose,
				NUM_BONES,
				&m_skeleton);

			vr::VRSettings()->GetString(c_settings_section, "left_arduino_port", m_arduino_port, sizeof(m_arduino_port));
		}

		m_active = true;

		m_pose_thread = std::thread(&Fngrs::UpdatePoseThread, this);

		m_skeletal_thread = std::thread(&Fngrs::ParseSerialThread, this);

		return vr::VRInitError_None;
	}
	vr::DriverPose_t GetPose() override { return m_pose; }
	void Deactivate() override
	{
		if (m_active)
		{
			m_active = false;
			m_pose_thread.join();
			m_skeletal_thread.join();
		}
		m_arduino_serial->~Serial();

		delete m_arduino_serial;

		m_arduino_serial = nullptr;
	}

	void EnterStandby() override {}
	void* GetComponent(const char* pchComponentNameAndVersion) override
	{
		DebugDriverLog("GetComponent called for %s", pchComponentNameAndVersion);
		return nullptr;
	}

	void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override {}

	// update the controller position based on tracking system.
	void UpdateControllerPose()
	{
		if (!m_found_controller) {

			m_controller_id = FindController(m_role == vr::TrackedControllerRole_RightHand);

			if (m_controller_id != -1) {
				m_found_controller = true;
				DebugDriverLog("Found controller with device id: %i", m_controller_id);
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}

		}
		else {
			vr::TrackedDevicePose_t hmd_pose[10];
			vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0, hmd_pose, 10);

			//Make sure that the pose is valid, if not, use a different way of tracking them
			if (hmd_pose[m_controller_id].bPoseIsValid)
			{
				vr::HmdMatrix34_t matrix = hmd_pose[m_controller_id].mDeviceToAbsoluteTracking;

				vr::HmdVector3_t vector_offset = MultiplyMatrix(Get33Matrix(matrix), m_offset_vector);

				m_pose.vecPosition[0] = hmd_pose[m_controller_id].mDeviceToAbsoluteTracking.m[0][3] + vector_offset.v[0];
				m_pose.vecPosition[1] = hmd_pose[m_controller_id].mDeviceToAbsoluteTracking.m[1][3] + vector_offset.v[1];
				m_pose.vecPosition[2] = hmd_pose[m_controller_id].mDeviceToAbsoluteTracking.m[2][3] + vector_offset.v[2]; //- forward

				vr::HmdQuaternion_t offset_quaternion = QuaternionFromAngle(1, 0, 0, DegToRad(-45));

				//merge rotation
				m_pose.qRotation = MultiplyQuaternion(GetRotation(matrix), offset_quaternion);

				m_pose.result = vr::TrackingResult_Running_OK;

				m_pose.vecAngularVelocity[0] = hmd_pose[m_controller_id].vAngularVelocity.v[0];
				m_pose.vecAngularVelocity[1] = hmd_pose[m_controller_id].vAngularVelocity.v[1];
				m_pose.vecAngularVelocity[2] = hmd_pose[m_controller_id].vAngularVelocity.v[2];

				m_pose.vecVelocity[0] = hmd_pose[m_controller_id].vVelocity.v[0];
				m_pose.vecVelocity[1] = hmd_pose[m_controller_id].vVelocity.v[1];
				m_pose.vecVelocity[2] = hmd_pose[m_controller_id].vVelocity.v[2];

				//set required properties
				m_pose.deviceIsConnected = true;

				m_pose.poseIsValid = true;

				//set the pose
				vr::VRServerDriverHost()->TrackedDevicePoseUpdated(m_id, m_pose, sizeof(vr::DriverPose_t));
			}
		}
	}

	int FindController(bool find_right_hand) {

		for (int i = 1; i < 10; i++) //omit id 0, as this is always the headset pose
		{
			vr::ETrackedPropertyError err;

			vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(i);

			std::string found_device_manufacturer = vr::VRProperties()->GetStringProperty(container, vr::Prop_ManufacturerName_String, &err);
			int32_t device_class = vr::VRProperties()->GetInt32Property(container, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32, &err);

			//We have a device that is not this one
			if (found_device_manufacturer != std::string(device_manufacturer)) {

				//We have a device which identifies itself as a tracked controller and is of the right hand
				if ((find_right_hand && device_class == vr::TrackedControllerRole_RightHand) || (!find_right_hand && device_class == vr::TrackedControllerRole_LeftHand)) {
					return i;
				}
			}
		}

		//We didn't find a controller
		return -1;
	}

	void ParseSerial() {
		if (!m_found_arduino) {
			m_arduino_serial = new Serial(m_arduino_port);

			if (m_arduino_serial->IsConnected()) {
				DebugDriverLog("Arduino connected successfully");
			}
			else {
				DebugDriverLog("Could not connected to arduino");
			}

			m_found_arduino = true;

			std::this_thread::sleep_for(std::chrono::milliseconds(1500));

			m_arduino_serial->PurgeBuffer();
		}

		char received_string[MAX_DATA_LENGTH];

		if (m_arduino_serial->IsConnected()) {
			//Read the data into the buffer
			int readResult = m_arduino_serial->ReadData(received_string, MAX_DATA_LENGTH);

			if (readResult != 0) {

				std::string str = received_string;
				std::string buf;
				std::stringstream ss(str);

				std::vector<std::string> tokens;

				while (getline(ss, buf, '&')) tokens.push_back(buf);

				//Index at which to update the bones, this starts at the root index finger bone.
				int bone_index = 6;
				int btn_index = 0;
				int current_index = 0;

				vr::VRBoneTransform_t hand_pose[NUM_BONES];

				//Copy our open hand poses into an array that we will manipulate
				std::copy(std::begin(m_role == vr::TrackedControllerRole_RightHand ? right_open_hand_pose : left_open_hand_pose), std::end(m_role == vr::TrackedControllerRole_RightHand ? right_open_hand_pose : left_open_hand_pose), std::begin(hand_pose));

				int fingers_in_fist = 0;

				for (std::string& it : tokens) {
					try {
						int result = std::stoi(it);
						//Skeletal data
						if (current_index < 4) {
							if (m_frame_count % 100 == 0) {
								DebugDriverLog("%i", result);
							}
							ComputeBoneTransform(hand_pose, (float)result / 1000.0f, bone_index, m_role == vr::TrackedControllerRole_RightHand);

							fingers_in_fist += result;
							bone_index += 5;
						}
						else {
							//Button data
							if (m_last_component_value[btn_index] != result) {
								//Joysticks
								if (btn_index == 0 || btn_index == 1) {
									float adjusted_result = -((float)result - 512.0f) / 512.0f;

									vr::VRDriverInput()->UpdateScalarComponent(m_h_component_values[btn_index], adjusted_result, 0);
								}
								//All other buttons
								else {
									vr::VRDriverInput()->UpdateBooleanComponent(m_h_component_values[btn_index], result, 0);
								}
								m_last_component_value[btn_index] = result;
							}
							btn_index++;
						}
					}
					catch (const std::exception& e) {
						DebugDriverLog("Exception caught while trying to convert to int. Skipping...");
					}
					current_index += 1;
				}

				vr::VRDriverInput()->UpdateSkeletonComponent(m_skeleton, vr::VRSkeletalMotionRange_WithoutController, hand_pose, NUM_BONES);
				vr::VRDriverInput()->UpdateSkeletonComponent(m_skeleton, vr::VRSkeletalMotionRange_WithController, hand_pose, NUM_BONES);

				if ((fingers_in_fist / 4) > 450) {
					vr::VRDriverInput()->UpdateBooleanComponent(m_h_component_values[6], 1, 0);
				}
				else {
					vr::VRDriverInput()->UpdateBooleanComponent(m_h_component_values[6], 0, 0);
				}
			}
		}
	}

	void UpdatePoseThread() {

		while (m_active) {
			UpdateControllerPose();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	void ParseSerialThread() {

		while (m_active) {
			ParseSerial();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			m_frame_count++;
		}
	}
};

class DeviceProvider : public vr::IServerTrackedDeviceProvider
{
public:
	Fngrs m_right_device;
	Fngrs m_left_device;

	vr::EVRInitError Init(vr::IVRDriverContext* pDriverContext) override
	{
		VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
		InitDriverLog(vr::VRDriverLog());

		m_right_device.Init(vr::TrackedControllerRole_RightHand);
		m_left_device.Init(vr::TrackedControllerRole_LeftHand);

		bool right_controler_activated = vr::VRServerDriverHost()->TrackedDeviceAdded(right_controller_serial, vr::TrackedDeviceClass_Controller, &m_right_device);
		bool left_controller_activated = vr::VRServerDriverHost()->TrackedDeviceAdded(left_controller_serial, vr::TrackedDeviceClass_Controller, &m_left_device);

		return vr::VRInitError_None;
	}

	void Cleanup() override {}
	const char* const* GetInterfaceVersions() override
	{
		return vr::k_InterfaceVersions;
	}
	void RunFrame() override {}
	bool ShouldBlockStandbyMode() override { return false; }
	void EnterStandby() override {}
	void LeaveStandby() override {}
} g_device_provider;

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C" 
#else
#error "Unsupported Platform."
#endif

HMD_DLL_EXPORT void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode)
{
	if (0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName))
	{
		return &g_device_provider;
	}
	DriverLog("HmdDriverFactory called for %s", pInterfaceName);
	return nullptr;
}
