# Corrscope 0.11.1 native trigger integration

Pinned upstream commit: 6838a0d30057543b928565df29c57ea50eba9359, https://github.com/corrscope/corrscope . Original complete source archive, Python reference modules, provenance and BSD license are kept here. `src/visualizer/Corrscope.cpp` translates CorrelationTrigger, get_period and LogFreqSpectrum for the upstream template defaults. FFT implementation, bounded latest-job worker and live-history/display adapter are host integration. Existing standard/legacy modes do not use this engine.

Native build: normal player CMake build; `corrscope_tests` is an optional native test target. Python/NumPy/Qt are not runtime dependencies. For the independent reference test, extract the original source ZIP into a test folder and install NumPy, attrs, atomicwrites, ruamel-yaml and appdirs into an isolated test directory. Run `tests/corrscope_reference.py <upstream-root> <dependency-dir> <corrscope_tests.exe> <output-dir>`. The test preserves exact differences and separately checks period-equivalent square-wave ties, not merely a correlation score. It uses generated signals only.

Display width/layout is the host UI's. No original GUI, all-project-settings interpreter, video renderer or user audio is embedded. See docs/CORRSCOPE-LANGUAGE-1.6.3.1h.json for comparison details and limitations.
