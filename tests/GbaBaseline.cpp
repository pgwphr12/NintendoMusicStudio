// Benchmark-only copy of the old redundant bootstrap. Never linked into the GUI.
#define NGMV_GBA_LEGACY_BOOTSTRAP
#define makePsfBackend makePsfBackendBaseline
#include "../src/formats/PsfBackend.cpp"
