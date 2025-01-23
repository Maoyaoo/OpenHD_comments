/******************************************************************************
 * OpenHD
 *
 * Licensed under the GNU General Public License (GPL) Version 3.
 *
 * This software is provided "as-is," without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose, and non-infringement. For details, see the
 * full license in the LICENSE file provided with this source code.
 *
 * Non-Military Use Only:
 * This software and its associated components are explicitly intended for
 * civilian and non-military purposes. Use in any military or defense
 * applications is strictly prohibited unless explicitly and individually
 * licensed otherwise by the OpenHD Team.
 *
 * Contributors:
 * A full list of contributors can be found at the OpenHD GitHub repository:
 * https://github.com/OpenHD
 *
 * © OpenHD, All Rights Reserved.
 ******************************************************************************/

#ifndef OPENHD_OPENHD_BUTTONS_H
#define OPENHD_OPENHD_BUTTONS_H

namespace openhd {

/**
 * Similar to openhd_led, openhd_buttons abstracts away hw-differences regarding
 * buttons. (Or stuff similar to buttons) Some functionalities might not be
 * supported on some hardware types, or a button might actually be a gpio jumper
 * for now.
 */
/**
 * 类似于 openhd_led，openhd_buttons 抽象了与按钮相关的硬件差异。
 * （或者类似按钮的东西）某些功能可能在某些硬件类型上不受支持，
 * 或者一个按钮实际上现在可能只是一个 GPIO 跳线。
 */
class ButtonManager {
   public:
    static ButtonManager& instance();
    /**
     * Called once at boot. Returns true if the button refering to the
     * 'Clean all settings / reset openhd core' functionality is pressed
     */
    /**
     * 在启动时调用一次。如果与“清除所有设置 / 重置 OpenHD 核心”功能相关的按钮被按下，则返回 true。
     */
    bool user_wants_reset_openhd_core();

   private:
    explicit ButtonManager() = default;
};

}  // namespace openhd
class openhd_buttons {};

#endif  // OPENHD_OPENHD_BUTTONS_H
