import ctypes
import ctypes.wintypes as wt
import json
import math
import time
import tomllib
from dataclasses import dataclass, field
from pathlib import Path

user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
dwmapi = ctypes.WinDLL("dwmapi")

DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = ctypes.c_void_p(-4)
USER_DEFAULT_SCREEN_DPI = 96
GA_ROOT = 2
GW_OWNER = 4
MONITOR_DEFAULTTONULL = 0
SW_MINIMIZE = 6
SWP_NOMOVE = 0x2
SWP_NOZORDER = 0x4
SWP_NOACTIVATE = 0x10
SW_RESTORE = 9
PW_CLIENTONLY = 1
PW_RENDERFULLCONTENT = 2
BI_RGB = 0
DIB_RGB_COLORS = 0
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
DWMWA_CLOAKED = 14

PT_TOUCH = 2
POINTER_FLAG_INRANGE = 0x2
POINTER_FLAG_INCONTACT = 0x4
POINTER_FLAG_CANCELED = 0x8000
POINTER_FLAG_DOWN = 0x10000
POINTER_FLAG_UPDATE = 0x20000
POINTER_FLAG_UP = 0x40000
TOUCH_MASK_CONTACTAREA = 0x1
TOUCH_MASK_ORIENTATION = 0x2
TOUCH_MASK_PRESSURE = 0x4
TOUCH_FEEDBACK = {"default": 1, "indirect": 2, "none": 3}

FLAGS_DOWN = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT
FLAGS_MOVE = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT
FLAGS_UP = POINTER_FLAG_UP
FLAGS_CANCEL = POINTER_FLAG_UP | POINTER_FLAG_CANCELED

DEFAULTS_PATH = Path(__file__).with_name("defaults.toml")


class POINTER_INFO(ctypes.Structure):
    _fields_ = [
        ("pointerType", wt.DWORD),
        ("pointerId", ctypes.c_uint32),
        ("frameId", ctypes.c_uint32),
        ("pointerFlags", ctypes.c_uint32),
        ("sourceDevice", wt.HANDLE),
        ("hwndTarget", wt.HWND),
        ("ptPixelLocation", wt.POINT),
        ("ptHimetricLocation", wt.POINT),
        ("ptPixelLocationRaw", wt.POINT),
        ("ptHimetricLocationRaw", wt.POINT),
        ("dwTime", wt.DWORD),
        ("historyCount", ctypes.c_uint32),
        ("InputData", ctypes.c_int32),
        ("dwKeyStates", wt.DWORD),
        ("PerformanceCount", ctypes.c_uint64),
        ("ButtonChangeType", ctypes.c_int),
    ]


class POINTER_TOUCH_INFO(ctypes.Structure):
    _fields_ = [
        ("pointerInfo", POINTER_INFO),
        ("touchFlags", ctypes.c_uint32),
        ("touchMask", ctypes.c_uint32),
        ("rcContact", wt.RECT),
        ("rcContactRaw", wt.RECT),
        ("orientation", ctypes.c_uint32),
        ("pressure", ctypes.c_uint32),
    ]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wt.DWORD),
        ("biWidth", wt.LONG),
        ("biHeight", wt.LONG),
        ("biPlanes", wt.WORD),
        ("biBitCount", wt.WORD),
        ("biCompression", wt.DWORD),
        ("biSizeImage", wt.DWORD),
        ("biXPelsPerMeter", wt.LONG),
        ("biYPelsPerMeter", wt.LONG),
        ("biClrUsed", wt.DWORD),
        ("biClrImportant", wt.DWORD),
    ]


WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

user32.SetProcessDpiAwarenessContext.argtypes = [ctypes.c_void_p]
user32.InitializeTouchInjection.argtypes = [ctypes.c_uint32, wt.DWORD]
user32.InjectTouchInput.argtypes = [ctypes.c_uint32, ctypes.POINTER(POINTER_TOUCH_INFO)]
user32.EnumWindows.argtypes = [WNDENUMPROC, wt.LPARAM]
user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
user32.GetAncestor.argtypes = [wt.HWND, ctypes.c_uint]
user32.GetAncestor.restype = wt.HWND
user32.WindowFromPoint.argtypes = [wt.POINT]
user32.WindowFromPoint.restype = wt.HWND
user32.GetForegroundWindow.restype = wt.HWND
user32.MonitorFromPoint.argtypes = [wt.POINT, wt.DWORD]
user32.MonitorFromPoint.restype = wt.HANDLE
user32.GetWindow.argtypes = [wt.HWND, ctypes.c_uint]
user32.GetWindow.restype = wt.HWND
user32.GetDpiForWindow.argtypes = [wt.HWND]
user32.GetDC.argtypes = [wt.HWND]
user32.GetDC.restype = wt.HDC
user32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
user32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
user32.IsIconic.argtypes = [wt.HWND]
user32.GetSystemMetrics.argtypes = [ctypes.c_int]
gdi32.CreateCompatibleDC.argtypes = [wt.HDC]
gdi32.CreateCompatibleDC.restype = wt.HDC
gdi32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
gdi32.CreateCompatibleBitmap.restype = wt.HBITMAP
gdi32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
gdi32.SelectObject.restype = wt.HGDIOBJ
gdi32.DeleteObject.argtypes = [wt.HGDIOBJ]
gdi32.DeleteDC.argtypes = [wt.HDC]
gdi32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
kernel32.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
kernel32.OpenProcess.restype = wt.HANDLE
kernel32.QueryFullProcessImageNameW.argtypes = [wt.HANDLE, wt.DWORD, wt.LPWSTR, ctypes.POINTER(wt.DWORD)]
kernel32.CloseHandle.argtypes = [wt.HANDLE]
dwmapi.DwmGetWindowAttribute.argtypes = [wt.HWND, wt.DWORD, ctypes.c_void_p, wt.DWORD]

SM_DIGITIZER = 94
SM_MAXIMUMTOUCHES = 95


class GuardError(RuntimeError):
    pass


class InjectError(RuntimeError):
    pass


def load_settings(path=None):
    with open(path or DEFAULTS_PATH, "rb") as f:
        return tomllib.load(f)


def make_dpi_aware():
    return bool(user32.SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))


def digitizer_info():
    return {"digitizer": user32.GetSystemMetrics(SM_DIGITIZER), "maximum_touches": user32.GetSystemMetrics(SM_MAXIMUMTOUCHES)}


def window_pid(hwnd):
    pid = wt.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    return pid.value


def process_name(pid):
    handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not handle:
        return None
    try:
        size = wt.DWORD(1024)
        buf = ctypes.create_unicode_buffer(size.value)
        if not kernel32.QueryFullProcessImageNameW(handle, 0, buf, ctypes.byref(size)):
            return None
        return Path(buf.value).name
    finally:
        kernel32.CloseHandle(handle)


def top_level_windows():
    found = []

    def visit(hwnd, _):
        if user32.IsWindowVisible(hwnd) and not user32.GetWindow(hwnd, GW_OWNER):
            found.append(hwnd)
        return True

    user32.EnumWindows(WNDENUMPROC(visit), 0)
    return found


def cloaked(hwnd):
    value = wt.DWORD()
    if dwmapi.DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, ctypes.byref(value), ctypes.sizeof(value)) != 0:
        return None
    return value.value


def window_rect(hwnd):
    r = wt.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    return [r.left, r.top, r.right, r.bottom]


def windows_of_processes(names):
    wanted = {n.lower() for n in names}
    out = []
    for hwnd in top_level_windows():
        name = process_name(window_pid(hwnd))
        if name and name.lower() in wanted:
            out.append({"process": name, "rect": window_rect(hwnd), "iconic": bool(user32.IsIconic(hwnd)), "cloaked": cloaked(hwnd)})
    return out


def find_window(pid):
    matches = [h for h in top_level_windows() if window_pid(h) == pid]
    if len(matches) != 1:
        raise GuardError(f"expected one visible top-level window for pid {pid}, found {len(matches)}")
    return matches[0]


@dataclass
class Target:
    pid: int
    hwnd: int

    @classmethod
    def of_pid(cls, pid):
        return cls(pid, find_window(pid))

    def scale(self):
        return user32.GetDpiForWindow(self.hwnd) / USER_DEFAULT_SCREEN_DPI

    def client_rect_screen(self):
        r = wt.RECT()
        user32.GetClientRect(self.hwnd, ctypes.byref(r))
        origin = wt.POINT(0, 0)
        user32.ClientToScreen(self.hwnd, ctypes.byref(origin))
        return origin.x, origin.y, r.right, r.bottom

    def info(self):
        x, y, w, h = self.client_rect_screen()
        s = self.scale()
        return {"pid": self.pid, "hwnd": self.hwnd, "scale": s, "client_px": [w, h], "client_dp": [w / s, h / s], "client_origin_px": [x, y]}

    def to_screen(self, x, y, unit):
        ox, oy, _, _ = self.client_rect_screen()
        k = self.scale() if unit == "dp" else 1.0
        return round(ox + x * k), round(oy + y * k)

    def is_foreground(self):
        return user32.GetForegroundWindow() == self.hwnd

    def _wait_foreground(self, wait_seconds):
        deadline = time.perf_counter() + wait_seconds
        while time.perf_counter() < deadline:
            if self.is_foreground():
                return True
            time.sleep(wait_seconds / 20)
        return self.is_foreground()

    def activate(self, wait_seconds):
        if self.is_foreground():
            return "already"
        user32.SetForegroundWindow(self.hwnd)
        if self._wait_foreground(wait_seconds):
            return "set_foreground"
        user32.ShowWindow(self.hwnd, SW_MINIMIZE)
        user32.ShowWindow(self.hwnd, SW_RESTORE)
        if self._wait_foreground(wait_seconds):
            return "minimize_restore"
        return None

    def resize_client(self, width, height, unit):
        k = self.scale() if unit == "dp" else 1.0
        outer = window_rect(self.hwnd)
        _, _, cw, ch = self.client_rect_screen()
        w = round(width * k) + (outer[2] - outer[0]) - cw
        h = round(height * k) + (outer[3] - outer[1]) - ch
        if not user32.SetWindowPos(self.hwnd, None, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE):
            raise InjectError(f"SetWindowPos failed ({ctypes.get_last_error()})")
        return self.info()

    def restore(self):
        return bool(user32.ShowWindow(self.hwnd, SW_RESTORE))

    def check(self, sx, sy):
        if find_window(self.pid) != self.hwnd:
            raise GuardError("the target window changed")
        if not self.is_foreground():
            raise GuardError("the target window is not the foreground window")
        if user32.IsIconic(self.hwnd):
            raise GuardError("the target window is minimized")
        ox, oy, w, h = self.client_rect_screen()
        if not (ox <= sx < ox + w and oy <= sy < oy + h):
            raise GuardError(f"({sx}, {sy}) is outside the client area {ox},{oy} {w}x{h}")
        pt = wt.POINT(sx, sy)
        if not user32.MonitorFromPoint(pt, MONITOR_DEFAULTTONULL):
            raise GuardError(f"({sx}, {sy}) is not on any monitor")
        under = user32.WindowFromPoint(pt)
        if not under or user32.GetAncestor(under, GA_ROOT) != self.hwnd:
            raise GuardError(f"({sx}, {sy}) is covered by another window")

    def capture(self, path):
        from PIL import Image

        _, _, w, h = self.client_rect_screen()
        if w <= 0 or h <= 0:
            raise InjectError("the client area is empty")
        screen_dc = user32.GetDC(self.hwnd)
        mem_dc = gdi32.CreateCompatibleDC(screen_dc)
        bmp = gdi32.CreateCompatibleBitmap(screen_dc, w, h)
        old = gdi32.SelectObject(mem_dc, bmp)
        try:
            if not user32.PrintWindow(self.hwnd, mem_dc, PW_CLIENTONLY | PW_RENDERFULLCONTENT):
                raise InjectError(f"PrintWindow failed ({ctypes.get_last_error()})")
            header = BITMAPINFOHEADER(ctypes.sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0)
            buf = ctypes.create_string_buffer(w * h * 4)
            if gdi32.GetDIBits(mem_dc, bmp, 0, h, buf, ctypes.byref(header), DIB_RGB_COLORS) != h:
                raise InjectError("GetDIBits failed")
        finally:
            gdi32.SelectObject(mem_dc, old)
            gdi32.DeleteObject(bmp)
            gdi32.DeleteDC(mem_dc)
            user32.ReleaseDC(self.hwnd, screen_dc)
        image = Image.frombuffer("RGBA", (w, h), buf.raw, "raw", "BGRA", 0, 1).convert("RGB")
        lo, hi = image.convert("L").getextrema()
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        image.save(path)
        return {"path": str(path), "size": [w, h], "uniform": lo == hi}


def ease(profile, t):
    if profile == "linear":
        return t
    if profile == "ease_out":
        return 1 - (1 - t) ** 2
    if profile == "ease_in":
        return t**2
    raise InjectError(f"unknown profile {profile}")


def wait_until(deadline):
    while True:
        left = deadline - time.perf_counter()
        if left <= 0:
            return
        if left > 0.002:
            time.sleep(left / 2)


@dataclass
class Injector:
    target: Target
    settings: dict
    log: list = field(default_factory=list)
    down: dict = field(default_factory=dict)

    def __post_init__(self):
        inj = self.settings["injection"]
        if not user32.InitializeTouchInjection(inj["max_contacts"], TOUCH_FEEDBACK[inj["feedback"]]):
            raise InjectError(f"InitializeTouchInjection failed ({ctypes.get_last_error()})")

    def _contact(self, pointer_id, sx, sy, flags):
        inj = self.settings["injection"]
        half = round(inj["contact_half_size_dp"] * self.target.scale())
        info = POINTER_TOUCH_INFO()
        info.pointerInfo.pointerType = PT_TOUCH
        info.pointerInfo.pointerId = pointer_id
        info.pointerInfo.pointerFlags = flags
        info.pointerInfo.ptPixelLocation = wt.POINT(sx, sy)
        info.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE
        info.rcContact = wt.RECT(sx - half, sy - half, sx + half, sy + half)
        info.orientation = inj["orientation"]
        info.pressure = inj["pressure"]
        return info

    def _send(self, contacts):
        arr = (POINTER_TOUCH_INFO * len(contacts))(*contacts)
        if not user32.InjectTouchInput(len(contacts), arr):
            raise InjectError(f"InjectTouchInput failed ({ctypes.get_last_error()})")

    def _frame(self, points, flags):
        for sx, sy in points.values():
            self.target.check(sx, sy)
        self._send([self._contact(pid, sx, sy, flags) for pid, (sx, sy) in points.items()])
        self.down = {} if flags == FLAGS_UP else dict(points)

    def _cancel(self):
        if self.down:
            try:
                self._send([self._contact(pid, sx, sy, FLAGS_CANCEL) for pid, (sx, sy) in self.down.items()])
                self.log.append({"cancelled": list(self.down.values())})
            except InjectError as e:
                self.log.append({"cancel_failed": str(e)})
            self.down = {}

    def gesture(self, paths, duration, hold_start, hold_end, profile, snapshots=(), out_dir=None):
        interval = self.settings["injection"]["frame_interval_seconds"]
        extension = self.settings["capture"]["extension"]
        pending = sorted(snapshots, key=lambda s: s["at"])
        taken = []
        ids = list(range(len(paths)))
        start_pts = {i: paths[i][0] for i in ids}
        live = dict(start_pts)
        frames = 0
        t0 = time.perf_counter()
        try:
            self._frame(live, FLAGS_DOWN)
            frames += 1
            phases = [(hold_start, None), (duration, "move"), (hold_end, None)]
            for length, kind in phases:
                begin = time.perf_counter()
                end = begin + length
                while True:
                    now = time.perf_counter()
                    if now >= end:
                        break
                    if kind == "move":
                        t = ease(profile, min(1.0, (now - begin) / length)) if length > 0 else 1.0
                        live = {i: self._lerp(paths[i], t) for i in ids}
                    self._frame(live, FLAGS_MOVE)
                    frames += 1
                    while pending and time.perf_counter() - t0 >= pending[0]["at"]:
                        snap = pending.pop(0)
                        taken.append(self.target.capture(Path(out_dir) / (snap["name"] + extension)))
                    wait_until(min(end, time.perf_counter() + interval))
                if kind == "move":
                    live = {i: paths[i][1] for i in ids}
                    self._frame(live, FLAGS_MOVE)
                    frames += 1
            t_up = time.perf_counter()
            self._frame(live, FLAGS_UP)
            frames += 1
        except Exception:
            self._cancel()
            raise
        result = {"frames": frames, "seconds": round(t_up - t0, 4)}
        if taken:
            result["snapshots"] = taken
        return result

    @staticmethod
    def _lerp(path, t):
        (ax, ay), (bx, by) = path
        return round(ax + (bx - ax) * t), round(ay + (by - ay) * t)


def run_step(injector, step, out_dir, events):
    target = injector.target
    timing = injector.settings["timing"]
    action = step["action"]
    unit = step.get("unit", "dp")
    record = {"step": step}
    if action in ("tap", "press"):
        p = target.to_screen(step["x"], step["y"], unit)
        hold = step.get("hold", timing["tap_hold_seconds"] if action == "tap" else timing["long_press_seconds"])
        record["screen"] = p
        record.update(injector.gesture([(p, p)], 0.0, hold, 0.0, "linear", step.get("snapshot", []), out_dir))
    elif action == "drag":
        paths = []
        for f, t in zip(step["from"], step["to"]) if isinstance(step["from"][0], list) else [(step["from"], step["to"])]:
            paths.append((target.to_screen(f[0], f[1], unit), target.to_screen(t[0], t[1], unit)))
        duration = step["duration"]
        profile = step.get("profile", "linear")
        record["screen"] = paths
        record.update(injector.gesture(paths, duration, step.get("hold_start", 0.0), step.get("hold_end", 0.0), profile, step.get("snapshot", []), out_dir))
        (ax, ay), (bx, by) = paths[0]
        k = target.scale()
        speed = math.hypot(bx - ax, by - ay) / k / duration if duration > 0 else 0.0
        record["mean_speed_dp_per_s"] = round(speed, 1)
    elif action == "wait":
        time.sleep(step.get("seconds", timing["settle_seconds"]))
    elif action == "capture":
        time.sleep(step.get("settle", timing["settle_seconds"]))
        record["capture"] = target.capture(Path(out_dir) / (step["name"] + injector.settings["capture"]["extension"]))
    elif action == "watch":
        record["windows"] = windows_of_processes(step.get("processes", injector.settings["watch"]["keyboard_processes"]))
    elif action == "resize":
        record["resized"] = target.resize_client(step["width"], step["height"], unit)
    elif action == "restore":
        record["restored"] = target.restore()
    elif action == "info":
        record["info"] = target.info()
    elif action == "activate":
        record["foreground"] = target.activate(timing["foreground_wait_seconds"])
        if not record["foreground"]:
            raise GuardError("the target window could not be brought to the foreground")
    else:
        raise InjectError(f"unknown action {action}")
    if action in ("tap", "press", "drag"):
        time.sleep(step.get("gap", timing["gesture_gap_seconds"]))
    events.append(record)
    return record


def run_scenario(pid, steps, out_dir, settings=None, echo=None):
    settings = settings or load_settings()
    make_dpi_aware()
    target = Target.of_pid(pid)
    injector = Injector(target, settings)
    events = [{"digitizer": digitizer_info(), "target": target.info()}]
    status = "ok"
    try:
        for step in steps:
            record = run_step(injector, step, out_dir, events)
            if echo:
                echo(record)
    except (GuardError, InjectError) as e:
        status = f"aborted: {type(e).__name__}: {e}"
        events.append({"error": status})
    events.extend(injector.log)
    Path(out_dir).mkdir(parents=True, exist_ok=True)
    return status, events


def write_log(path, events):
    with open(path, "w", encoding="utf-8") as f:
        for e in events:
            f.write(json.dumps(e, ensure_ascii=False) + "\n")
