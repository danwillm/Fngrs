#pragma once
#include <openvr_driver.h>

const int NUM_BONES = 31;
extern vr::VRBoneTransform_t right_open_hand_pose[NUM_BONES];
extern vr::VRBoneTransform_t right_fist_pose[NUM_BONES];

extern vr::VRBoneTransform_t left_open_hand_pose[NUM_BONES];
extern vr::VRBoneTransform_t left_fist_pose[NUM_BONES];

void ComputeBoneTransform(vr::VRBoneTransform_t bone_transform[NUM_BONES], float transform, int bone_index, bool right_hand);
float Lerp(float a, float b, float f);