# Third-party notices

MiniMetersMobile is distributed under the GNU General Public License v3 (see `LICENSE`). It builds
on the following components.

## JUCE

- Version: 7.0.7 (git submodule, used unmodified)
- Upstream: https://github.com/juce-framework/JUCE
- Licence: dual — GNU GPL v3, or a commercial JUCE licence
- This project uses JUCE under the **GPL v3** option. Because the whole work is GPLv3, the
  corresponding source is published at
  https://github.com/SOMWHY/MiniMetersMobileServer
- JUCE's `juce_audio_basics`, `juce_audio_devices`, `juce_core` and `juce_events` modules are
  additionally available under the ISC licence.

## VST 3 SDK

- Vendored inside JUCE (`modules/juce_audio_processors/format_types/VST3_SDK`)
- Upstream: https://github.com/steinbergmedia/vst3sdk
- Licence: dual — GNU GPL v3, or the proprietary Steinberg VST 3 Licence
- This project uses the **GPL v3** option. "VST" is a registered trademark of Steinberg Media
  Technologies GmbH; this project is not affiliated with or endorsed by Steinberg.

## CLAP

- Version: as pinned by the `clap-juce-extensions` submodule
- Upstream: https://github.com/free-audio/clap, https://github.com/free-audio/clap-juce-extensions
- Licence: MIT (clap-juce-extensions: MIT, Copyright 2019-2020 Paul Walker)

## QR Code generator library

- Upstream: https://github.com/nayuki/QR-Code-generator
- Licence: MIT, Copyright (c) Project Nayuki

## JSON for Modern C++

- Version: 3.11.1, vendored as `json.hpp`
- Upstream: https://github.com/nlohmann/json
- Licence: MIT, SPDX-FileCopyrightText: 2013-2022 Niels Lohmann

## cpp-httplib

- Vendored as `httplib.h`; **not linked** in the current build (the include is commented out in
  `PluginProcessor.h`). Included here for completeness.
- Upstream: https://github.com/yhirose/cpp-httplib
- Licence: MIT, Copyright (c) 2017 Yhirose
