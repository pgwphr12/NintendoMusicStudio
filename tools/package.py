# Compatibility entry point for documented/CI packaging commands.
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name('package_player.py')),run_name='__main__')
