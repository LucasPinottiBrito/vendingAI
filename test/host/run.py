# Run from a Visual Studio developer prompt, or with g++ on PATH.
from pathlib import Path
import tempfile
import subprocess
import shutil
import os
here = Path(__file__).resolve().parent
src = here.parents[1] / "src"
headers = ["driver/gpio.h","esp_err.h","esp_timer.h","esp_rom_sys.h","esp_log.h","nvs.h",
           "nvs_flash.h","freertos/FreeRTOS.h","freertos/semphr.h","freertos/task.h"]
sources = [here/"cycles.cpp", src/"motor/MotorController.cpp", src/"sensor/DropDetector.cpp",
           src/"sensor/IrBreakBeamSensor.cpp", src/"vending/VendingController.cpp"]
with tempfile.TemporaryDirectory() as d:
    root = Path(d)
    for name in headers:
        path = root/name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "fake_idf.hpp"\n')
    exe = root/("cycles.exe" if os.name == "nt" else "cycles")
    if shutil.which("cl"):
        cmd = ["cl","/nologo","/EHsc","/std:c++17",f"/Fe:{exe}"]
    else:
        cmd = ["g++","-std=c++17","-o",str(exe)]
    subprocess.run(cmd+[f"-I{x}" for x in (root,here,src)]+list(map(str,sources)),cwd=root,check=True)
    subprocess.run([str(exe)],check=True)
