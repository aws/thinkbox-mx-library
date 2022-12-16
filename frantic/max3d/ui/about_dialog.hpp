// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>

namespace frantic {
namespace max3d {
namespace ui {

/**
 * Show an About dialog box.
 *
 * @param hInstance Handle of the module that contains the dialog box template
 *                  (i.e, your plugin module).
 * @param title Dialog box title.
 * @param productName Product name.
 * @param version Product version number.
 * @param notices Product's copyright notices and attributions.  This may be
 *                many lines long, so it appears in a scrollable widget.
 */
void show_about_dialog( HINSTANCE hInstance, const frantic::tstring& title, const frantic::tstring& productName,
                        const frantic::tstring& version, const frantic::tstring& notices );

} // namespace ui
} // namespace max3d
} // namespace frantic
