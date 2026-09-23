#!/usr/bin/env python3

import argparse
import re
import sys
import requests
import xml.etree.ElementTree as ET


class HikvisionPTZ:
    def __init__(self, ip, username, password, channel=1):
        self.base_url = f"http://{ip}"
        self.username = username
        self.password = password
        self.channel = channel

        self.url = (
            f"{self.base_url}/ISAPI/PTZCtrl/"
            f"channels/{self.channel}/absoluteEx"
        )

        self.cap_url = (
            f"{self.base_url}/ISAPI/PTZCtrl/"
            f"channels/{self.channel}/absoluteEx/capabilities"
        )

        self.session = requests.Session()
        self.session.auth = requests.auth.HTTPDigestAuth(
            self.username,
            self.password
        )

    def _request(self, method, url, data=None):
        try:
            response = self.session.request(
                method,
                url,
                data=data,
                headers={
                    "Content-Type": "application/xml"
                },
                timeout=5
            )

            if response.status_code != 200:
                print(
                    f"HTTP错误: {response.status_code}",
                    file=sys.stderr
                )
                print(response.text, file=sys.stderr)
                return None

            return response.text

        except requests.RequestException as e:
            print(f"网络请求失败: {e}", file=sys.stderr)
            return None

    def get_position(self):
        """
        获取当前PTZ位置
        """

        response = self._request("GET", self.url)

        if response is None:
            return None

        try:
            root = ET.fromstring(response)

            elevation = root.find(
                ".//{http://www.isapi.org/ver20/XMLSchema}elevation"
            )

            azimuth = root.find(
                ".//{http://www.isapi.org/ver20/XMLSchema}azimuth"
            )

            zoom = root.find(
                ".//{http://www.isapi.org/ver20/XMLSchema}absoluteZoom"
            )

            if elevation is None or azimuth is None:
                print("无法解析PTZ位置", file=sys.stderr)
                print(response, file=sys.stderr)
                return None

            return {
                "elevation": float(elevation.text),
                "azimuth": float(azimuth.text),
                "zoom": float(zoom.text) if zoom is not None else None
            }

        except ET.ParseError as e:
            print(f"XML解析失败: {e}", file=sys.stderr)
            print(response, file=sys.stderr)
            return None

    def get_capabilities(self):
        """
        获取PTZ能力范围
        """

        response = self._request("GET", self.cap_url)

        if response is None:
            return None

        try:
            root = ET.fromstring(response)

            elevation = root.find(
                ".//{http://www.isapi.org/ver20/XMLSchema}elevation"
            )

            azimuth = root.find(
                ".//{http://www.isapi.org/ver20/XMLSchema}azimuth"
            )

            if elevation is None or azimuth is None:
                print("无法解析PTZ能力", file=sys.stderr)
                return None

            return {
                "elevation_min": float(elevation.attrib["min"]),
                "elevation_max": float(elevation.attrib["max"]),
                "azimuth_min": float(azimuth.attrib["min"]),
                "azimuth_max": float(azimuth.attrib["max"])
            }

        except Exception as e:
            print(f"解析PTZ能力失败: {e}", file=sys.stderr)
            return None

    def set_position(self, elevation=None, azimuth=None):
        """
        设置绝对PTZ位置
        """

        current = self.get_position()

        if current is None:
            return False

        if elevation is None:
            elevation = current["elevation"]

        if azimuth is None:
            azimuth = current["azimuth"]

        zoom = current["zoom"]

        xml = f"""<?xml version="1.0" encoding="UTF-8"?>
<PTZAbsoluteEx version="2.0"
xmlns="http://www.isapi.org/ver20/XMLSchema">
    <elevation>{elevation:.2f}</elevation>
    <azimuth>{azimuth:.2f}</azimuth>
"""

        if zoom is not None:
            xml += f"    <absoluteZoom>{zoom:.2f}</absoluteZoom>\n"

        xml += """</PTZAbsoluteEx>"""

        print()
        print("发送PTZ控制:")
        print(f"  elevation = {elevation:.2f}")
        print(f"  azimuth   = {azimuth:.2f}")

        response = self._request(
            "PUT",
            self.url,
            data=xml
        )

        if response is None:
            return False

        print("PTZ控制成功")

        return True

    def move_up(self, degree):
        """
        向上旋转degree度
        """

        current = self.get_position()

        if current is None:
            return False

        target = current["elevation"] + degree

        capabilities = self.get_capabilities()

        if capabilities:
            target = min(
                target,
                capabilities["elevation_max"]
            )

        print(
            f"向上 {degree}°: "
            f"{current['elevation']:.2f}° -> {target:.2f}°"
        )

        return self.set_position(
            elevation=target
        )

    def move_down(self, degree):
        """
        向下旋转degree度
        """

        current = self.get_position()

        if current is None:
            return False

        target = current["elevation"] - degree

        capabilities = self.get_capabilities()

        if capabilities:
            target = max(
                target,
                capabilities["elevation_min"]
            )

        print(
            f"向下 {degree}°: "
            f"{current['elevation']:.2f}° -> {target:.2f}°"
        )

        return self.set_position(
            elevation=target
        )

    def move_left(self, degree):
        """
        向左旋转degree度

        azimuth:
            0 ~ 360
        """

        current = self.get_position()

        if current is None:
            return False

        target = current["azimuth"] - degree

        # 处理360度环绕
        while target < 0:
            target += 360

        while target >= 360:
            target -= 360

        print(
            f"向左 {degree}°: "
            f"{current['azimuth']:.2f}° -> {target:.2f}°"
        )

        return self.set_position(
            azimuth=target
        )

    def move_right(self, degree):
        """
        向右旋转degree度

        azimuth:
            0 ~ 360
        """

        current = self.get_position()

        if current is None:
            return False

        target = current["azimuth"] + degree

        # 处理360度环绕
        while target < 0:
            target += 360

        while target >= 360:
            target -= 360

        print(
            f"向右 {degree}°: "
            f"{current['azimuth']:.2f}° -> {target:.2f}°"
        )

        return self.set_position(
            azimuth=target
        )


def main():

    parser = argparse.ArgumentParser(
        description="Hikvision PTZ Controller"
    )

    parser.add_argument(
        "--ip",
        default="192.168.44.64"
    )

    parser.add_argument(
        "--user",
        default="admin"
    )

    parser.add_argument(
        "--password",
        default="yuanqi456"
    )

    parser.add_argument(
        "--channel",
        type=int,
        default=1
    )

    sub = parser.add_subparsers(
        dest="command"
    )

    # 查看当前位置
    sub.add_parser("get")

    # 查看能力
    sub.add_parser("capabilities")

    # 上
    up = sub.add_parser("up")
    up.add_argument(
        "degree",
        type=float
    )

    # 下
    down = sub.add_parser("down")
    down.add_argument(
        "degree",
        type=float
    )

    # 左
    left = sub.add_parser("left")
    left.add_argument(
        "degree",
        type=float
    )

    # 右
    right = sub.add_parser("right")
    right.add_argument(
        "degree",
        type=float
    )

    # 设置绝对角度
    absolute = sub.add_parser("set")
    absolute.add_argument(
        "--elevation",
        type=float
    )
    absolute.add_argument(
        "--azimuth",
        type=float
    )

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    ptz = HikvisionPTZ(
        ip=args.ip,
        username=args.user,
        password=args.password,
        channel=args.channel
    )

    if args.command == "get":

        position = ptz.get_position()

        if position:
            print()
            print("当前PTZ:")
            print(
                f"  上下角度 elevation : "
                f"{position['elevation']:.2f}°"
            )
            print(
                f"  左右角度 azimuth   : "
                f"{position['azimuth']:.2f}°"
            )
            print(
                f"  Zoom               : "
                f"{position['zoom']}"
            )

    elif args.command == "capabilities":

        capabilities = ptz.get_capabilities()

        if capabilities:
            print()
            print("PTZ能力:")
            print(
                f"  elevation: "
                f"{capabilities['elevation_min']}° ~ "
                f"{capabilities['elevation_max']}°"
            )
            print(
                f"  azimuth: "
                f"{capabilities['azimuth_min']}° ~ "
                f"{capabilities['azimuth_max']}°"
            )

    elif args.command == "up":

        ptz.move_up(args.degree)

    elif args.command == "down":

        ptz.move_down(args.degree)

    elif args.command == "left":

        ptz.move_left(args.degree)

    elif args.command == "right":

        ptz.move_right(args.degree)

    elif args.command == "set":

        ptz.set_position(
            elevation=args.elevation,
            azimuth=args.azimuth
        )


if __name__ == "__main__":
    main()