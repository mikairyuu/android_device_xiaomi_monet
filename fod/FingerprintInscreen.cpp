/*
 * Copyright (C) 2019-2020 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "FingerprintInscreenService"

#include <android-base/properties.h>
#include "FingerprintInscreen.h"
#include <android-base/logging.h>
#include <hardware_legacy/power.h>
#include <cmath>
#include <fstream>

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define TOUCH_FOD_ENABLE 10
#define TOUCH_AOD_ENABLE 11

#define FOD_POS_X 445
#define FOD_POS_Y 2025
#define FOD_SIZE 190

#define BRIGHTNESS_PATH "/sys/class/backlight/panel0-backlight/brightness"
#define FINGERPRINT_ACQUIRED_VENDOR 6

namespace vendor {
namespace lineage {
namespace biometrics {
namespace fingerprint {
namespace inscreen {
namespace V1_0 {
namespace implementation {

/*
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

FingerprintInscreen::FingerprintInscreen() {
    xiaomiDisplayFeatureService = IDisplayFeature::getService();
    touchFeatureService = ITouchFeature::getService();
    xiaomiFingerprintService = IXiaomiFingerprint::getService();
}

Return<void> FingerprintInscreen::onStartEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onFinishEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onPress() {
    acquire_wake_lock(PARTIAL_WAKE_LOCK, LOG_TAG);
    xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_FOD);
    return Void();
}

Return<void> FingerprintInscreen::onRelease() {
    xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_NONE);
    release_wake_lock(LOG_TAG);
    return Void();
}

Return<void> FingerprintInscreen::onShowFODView() {
    touchFeatureService->setTouchMode(TOUCH_FOD_ENABLE, 1);
    return Void();
}

Return<void> FingerprintInscreen::onHideFODView() {
    touchFeatureService->resetTouchMode(TOUCH_FOD_ENABLE);
    xiaomiDisplayFeatureService->setFeature(0, 17, 0, 1);
    return Void();
}

Return<bool> FingerprintInscreen::handleAcquired(int32_t acquiredInfo, int32_t vendorCode) {
    std::lock_guard<std::mutex> _lock(mCallbackLock);
    if (mCallback == nullptr) {
        return false;
    }

    if (acquiredInfo == FINGERPRINT_ACQUIRED_VENDOR) {
        if(vendorCode == 22) {
           Return<void> ret = mCallback->onFingerDown();
           if (!ret.isOk()) {
               LOG(ERROR) << "FingerDown() error: " << ret.description();
           }
           return true;
        }

        if (vendorCode == 23) {
           Return<void> ret = mCallback->onFingerUp();
           if (!ret.isOk()) {
               LOG(ERROR) << "FingerUp() error: " << ret.description();
           }
           return true;
        }
    }

    return false;
}

Return<bool> FingerprintInscreen::handleError(int32_t error, int32_t vendorCode) {
    LOG(ERROR) << "error: " << error << ", vendorCode: " << vendorCode;
    return false;
}

Return<void> FingerprintInscreen::setLongPressEnabled(bool) {
    return Void();
}


Return<bool> FingerprintInscreen::shouldBoostBrightness() {
    return false;
}

Return<void> FingerprintInscreen::setCallback(const sp<IFingerprintInscreenCallback>& callback) {
   {
        std::lock_guard<std::mutex> _lock(mCallbackLock);
        mCallback = callback;
   }
    return Void();
}

Return<int32_t> FingerprintInscreen::getPositionX() {
    return FOD_POS_X;
}

Return<int32_t> FingerprintInscreen::getPositionY() {
    return FOD_POS_Y;
}
Return<int32_t> FingerprintInscreen::getDimAmount(int32_t) {
    return 0;

}

Return<int32_t> FingerprintInscreen::getSize() {
    return FOD_SIZE;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace inscreen
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace lineage
}  // namespace vendor
