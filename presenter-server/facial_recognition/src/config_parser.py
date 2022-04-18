"""人脸识别配置解析模块"""

import os
import logging
import configparser
import common.parameter_validation as validate


def config_verify():
    """ 配置参数合法性验证 """
    if not validate.validate_ip(ConfigParser.web_server_ip) or \
            not validate.validate_ip(ConfigParser.presenter_server_ip) or \
            not validate.validate_port(ConfigParser.web_server_port) or \
            not validate.validate_port(ConfigParser.presenter_server_port):
        return False

    if not validate.validate_integer(ConfigParser.max_face_num, 0, 10000):
        print("人脸库支持的人脸数量为1-10000.")
        logging.warning("最大人脸数量应为1-10000.")
        return False

    if not validate.validate_float(ConfigParser.face_match_threshold, 0, 1):
        print("人脸匹配阈值应为 0-1.")
        logging.warning("人脸匹配阈值应为 0-1.")
        return False

    if not os.path.isdir(ConfigParser.storage_dir):
        print("您应该手动创建目录 \"%s\".", ConfigParser.storage_dir)
        logging.warning("需要手动创建存储人脸照片的目录 \"%s\".", ConfigParser.storage_dir)
        return False

    return True


class ConfigParser:
    """ 从 config.conf 解析配置"""
    __instance = None   # 单实例
    root_path = None    # 根路径

    # Socket服务器地址和Http服务器地址
    presenter_server_ip = None
    presenter_server_port = None
    web_server_ip = None
    web_server_port = None

    max_face_num = None    # 最大人脸数
    face_match_threshold = None # 人脸识别阈值
    storage_dir = None  # 人脸图片存储路径

    def __init__(self):
        """初始化"""

    def __new__(cls):
        """确保类对象是单个实例"""
        if cls.__instance is None:
            cls.__instance = object.__new__(cls)
            cls.config_parser()
        return cls.__instance

    @classmethod
    def config_parser(cls):
        """ 对config/config.conf配置文件的解析"""
        # 创建解析器类实例
        config_parser = configparser.ConfigParser()
        # 读取配置文件
        cls.root_path = ConfigParser.get_root_path()
        config_file = os.path.join(cls.root_path, "config/config.conf")
        config_parser.read(config_file)

        # 读取配置文件中的参数
        cls.presenter_server_ip = config_parser.get('baseconf', 'presenter_server_ip')
        cls.presenter_server_port = config_parser.get('baseconf', 'presenter_server_port')
        cls.web_server_ip = config_parser.get('baseconf', 'web_server_ip')
        cls.web_server_port = config_parser.get('baseconf', 'web_server_port')
        cls.storage_dir = config_parser.get('baseconf', 'storage_dir')
        cls.max_face_num = config_parser.get('baseconf', 'max_face_num')
        cls.face_match_threshold = config_parser.get('baseconf', 'face_match_threshold')

    @staticmethod
    def get_root_path():
        """获取Socket服务器的根目录，至src"""
        path = __file__
        idx = path.rfind("src")

        return path[0:idx]
