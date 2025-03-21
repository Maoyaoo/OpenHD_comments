#!/bin/bash

################################################################################
# OpenHD
# 
# Licensed under the GNU General Public License (GPL) Version 3.
# 
# This software is provided "as-is," without warranty of any kind, express or 
# implied, including but not limited to the warranties of merchantability, 
# fitness for a particular purpose, and non-infringement. For details, see the 
# full license in the LICENSE file provided with this source code.
# 
# Non-Military Use Only:
# This software and its associated components are explicitly intended for 
# civilian and non-military purposes. Use in any military or defense 
# applications is strictly prohibited unless explicitly and individually 
# licensed otherwise by the OpenHD Team.
# 
# Contributors:
# A full list of contributors can be found at the OpenHD GitHub repository:
# https://github.com/OpenHD
# 
# © OpenHD, All Rights Reserved.
###############################################################################

# OpenHD clang-format checking.
# Performed steps:
# Step1: Generate list of all .cpp / .h / .hpp files of this project
#        (excluding subdirectories)
# Step 2: Run clang-format
# Arguments: run with 'f' to fix things (otherwise, default, only check and report
# error if clang-format finds any issues

#penHD 代码风格检查（基于 clang-format）
#执行步骤：
#步骤 1：生成本项目所有.cpp/.h/.hpp 文件的列表（不包括子目录）
#步骤 2：运行 clang-format 工具
#参数说明：使用参数 “f” 可自动修复代码风格问题（否则，默认仅进行检查，若 clang-format 发现任何问题则报告错误）

function append_all_sources_headers() {
    # We use .h, .cpp and .hpp in OpenHD
    TMP_FILE_LIST="$(find "$1" | grep -E ".*(\.cpp|\.h|\.hpp)$")"
    #TMP_FILE_LIST+='\n'
    FILE_LIST+=$'\n'
    FILE_LIST+=$TMP_FILE_LIST
}


THIS_PATH="$(realpath "$0")"
THIS_DIR="$(dirname "$THIS_PATH")"


append_all_sources_headers "$THIS_DIR/ohd_common/inc"
append_all_sources_headers "$THIS_DIR/ohd_common/src"
append_all_sources_headers "$THIS_DIR/ohd_common/test"

append_all_sources_headers "$THIS_DIR/ohd_interface/inc"
append_all_sources_headers "$THIS_DIR/ohd_interface/src"
append_all_sources_headers "$THIS_DIR/ohd_interface/test"

append_all_sources_headers "$THIS_DIR/ohd_telemetry/src"
append_all_sources_headers "$THIS_DIR/ohd_telemetry/test"

append_all_sources_headers "$THIS_DIR/ohd_video/inc"
append_all_sources_headers "$THIS_DIR/ohd_video/src"

echo "Files found to format = \n\"\"\"\n$FILE_LIST\n\"\"\""

# Checks for clang-format issues and returns error if they exist
function check_warning(){
  clang-format --dry-run --Werror --verbose -i --style=file $FILE_LIST

  if [ "$?" -eq "0" ]; then
    echo "Everything formatted correctly"
  else
    echo "There are formatting errors ! Please fix first."
    exit 1
  fi
}

# fixes any issues (re-formats everything)
function fix_warnings() {
    clang-format --verbose -i --style=file $FILE_LIST
}

if [ "$1" == "f" ]; then
   echo "Fixing warnings"
   fix_warnings
else
  echo "Checking warnings"
  check_warning
fi
