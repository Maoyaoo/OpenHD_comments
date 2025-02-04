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

#ifndef OPENHD_OPENHD_OHD_TELEMETRY_SRC_ROUTING_MAVLINKSYSTEM_H_
#define OPENHD_OPENHD_OHD_TELEMETRY_SRC_ROUTING_MAVLINKSYSTEM_H_

#include <map>
#include <memory>
#include <vector>

/**
 * A system has a sys id and allows adding components.
 */
// 这段代码定义了一个名为 MavlinkSystem 的类，用于表示一个 MAVLink 系统。
class MavlinkSystem {
   public:
    // 这是一个显式构造函数，用于初始化 MavlinkSystem 对象。explicit 关键字:防止隐式类型转换，确保只能显式调用构造函数
    explicit MavlinkSystem(uint8_t sys_id) : _sys_id(sys_id) {}

    // 这是一个常量成员变量，表示 MAVLink 系统的 ID,取值范围为 0 到 255。
    // 系统 ID 是 MAVLink 协议中用于唯一标识一个系统的值。
    const uint8_t _sys_id;
    /*void add_component(std::shared_ptr<MavlinkComponent> component){
      if(components.find(component->_comp_id)!=components.end()){
        std::cerr<<"Error Component already exists";
        return;
      }
      components.insert_or_assign(component->_comp_id,component);
    }
    std::map<uint8_t,std::shared_ptr<MavlinkComponent>> components;*/
};

#endif  // OPENHD_OPENHD_OHD_TELEMETRY_SRC_ROUTING_MAVLINKSYSTEM_H_
