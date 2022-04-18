import os
import sys
import signal
import argparse
import logging


WEB_SERVER = None
APP_SERVER = None
RUN_SERVER = None
SERVER_TYPE = ""
USAGE_INFO = "python3.7.5 prensenter_server.py [-h] --app \n\t\t\t\t{facial_recognition}"

FACIAL_RECOGNITION_MAP = {"web_server": "facial_recognition.src.web",
                          "app_server": "facial_recognition.src.facial_recognition_server"
                          }

APP_CONF_MAP = {"facial_recognition": FACIAL_RECOGNITION_MAP}


def arg_parse():
    """参数解析"""
    global WEB_SERVER
    global APP_SERVER
    global SERVER_TYPE

    # 创建解析器
    parser = argparse.ArgumentParser(usage=USAGE_INFO)
    parser.add_argument('--app', type=str, required=True,
                        choices=['facial_recognition'],
                        help="人脸识别展示应用.")
    # 解析参数
    args = parser.parse_args()
    SERVER_TYPE = args.app

    # 动态加载模块
    app_conf = APP_CONF_MAP.get(SERVER_TYPE)
    WEB_SERVER = __import__(app_conf.get("web_server"), fromlist=True)
    APP_SERVER = __import__(app_conf.get("app_server"), fromlist=True)


def check_server_exist():
    # 获取进程ID
    pid = os.getpid()

    cmd = "ps -ef|grep -v {}|grep -w presenter_server|grep {}" .format(pid, SERVER_TYPE)
    ret = os.system(cmd)
    return ret


def start_app():
    global RUN_SERVER
    # 启动Socket服务器
    RUN_SERVER = APP_SERVER.run()
    if RUN_SERVER is None:
        return False

    logging.info("Socket服务器启动成功,类型为: %s", SERVER_TYPE)
    # 启动Web服务器
    return WEB_SERVER.start_webapp()


def close_all_thread(signum, frame):
    """关闭所有线程"""
    WEB_SERVER.stop_webapp()
    RUN_SERVER.stop_thread()
    logging.info("Ctrl+c使得应用退出")

    sys.exit()


def main_process():
    """主程序入口"""
    arg_parse()

    if check_server_exist() == 0:
        print("应用类型 \"%s\" 已存在!" % SERVER_TYPE)
        return True

    signal.signal(signal.SIGINT, close_all_thread)
    signal.signal(signal.SIGTERM, close_all_thread)
    start_app()

    return True


if __name__ == "__main__":
    main_process()
