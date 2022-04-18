"""Socket服务器模块"""

import os
import threading
import random
import logging
from logging.config import fileConfig
import numpy as np
from json.decoder import JSONDecodeError
from google.protobuf.message import DecodeError

import common.presenter_message_pb2 as presenter_message_pb2
from common.channel_manager import ChannelManager
from common.presenter_socket_server import PresenterSocketServer
from common.app_manager import AppManager

import facial_recognition.src.facial_recognition_message_pb2 as pb2
from facial_recognition.src.config_parser import ConfigParser, config_verify
from facial_recognition.src.facial_recognition_handler import FacialRecognitionHandler
from facial_recognition.src.mysql_handle import MysqlHandle

# 人脸注册超时时间为10秒
FACE_REGISTER_TIME_OUT = 10

# 最多支持2个应用程序连接
MAX_APP_NUM = 1

# 人脸特征向量的长度
FEATURE_VECTOR_LENGTH = 1024

# 人脸注册状态码
FACE_REGISTER_STATUS_WAITING = 1
FACE_REGISTER_STATUS_SUCCEED = 2
FACE_REGISTER_STATUS_FAILED = 3


def _parse_protobuf(protobuf, msg_data):
    """
    Description:  解析protobuf，即将消息体反序列化到protobuf中
    Input:
        protobuf: protobuf 定义的结构体
        msg_data: 消息体，由protobuf序列化
    Returns: True or False
    """
    try:
        protobuf.ParseFromString(msg_data)
        return True
    except DecodeError as exp:
        logging.error(exp)
        return False


def _compute_similar_degree(feature_vector1, feature_vector2):
    """
    Description: 计算两个向量的余弦相似度
    Input:
        feature_vector1: 人脸特征向量1
        feature_vector2: 人脸特征向量2
    Returns: 分数
    """
    vector1 = np.array(feature_vector1)
    vector2 = np.array(feature_vector2)
    square_diff = ((np.linalg.norm(vector1)) * (np.linalg.norm(vector2)))
    score = np.dot(vector1, vector2) / square_diff
    return score


class FacialRecognitionServer(PresenterSocketServer):
    """人脸识别&人脸注册Socket服务器"""

    def __init__(self, config):
        """
        Description: 类初始化函数
        Input:
            config: 配置信息
        """
        # 服务器地址
        server_address = (config.presenter_server_ip,int(config.presenter_server_port))
        super(FacialRecognitionServer, self).__init__(server_address)

        # 最大支持人脸数和人脸匹配阈值
        self.max_face_num = int(config.max_face_num)
        self.face_match_threshold = float(config.face_match_threshold)
        # 本地存储人脸图片的路径
        self.storage_dir = config.storage_dir

        # MySQL数据库处理类实例，app管理类实例和通道管理类实例
        self.mysqlHandle = MysqlHandle()
        self.app_manager = AppManager()
        self.channel_manager = ChannelManager()

        # app相关参数
        self.device_id = None # app_id为设备名称，device_id为设备ID
        self.app_dict = {}  # 内容为：app_id(device name) : id

        # 人脸注册信息
        self.register_dict = {} # 注册字典

        # 人脸信息库数据
        self.registered_faces = None
        self.face_lock = threading.Lock()

        # 初始化人脸信息库
        self._init_face_database()

    def _init_face_database(self):
        """
        Description: 初始化人脸识别数据库,从MySQL数据库读取人脸信息
        """
        self.registered_faces = self.mysqlHandle.get_all_face_feature()

    def _clean_connect(self, sock_fileno, epoll, conns, msgs):
        """
        Description: 关闭套接字，并清除局部变量
        Input:
            sock_fileno: 一个套接字文件号，socket.fileno()的返回值
            epoll: set类型，select.epoll。
            conns: 在epoll中注册的所有套接字连接
            msgs: 从套接字读取得到的消息
        """
        logging.info("清空fd:%s, conns:%s", sock_fileno, conns)
        self.app_manager.unregister_app_by_fd(sock_fileno)
        epoll.unregister(sock_fileno)
        conns[sock_fileno].close()
        del conns[sock_fileno]
        del msgs[sock_fileno]
        # 设置设备运行状态为stop
        self.mysqlHandle.unregister_app(self.device_id)

    def _process_msg(self, conn, msg_name, msg_data):
        """
        处理protobuf msg的总入口
        Input:
            conn: 套接字连接
            msg_name: 消息的名称。
            msg_data: 消息体，由protobuf序列化

        Returns:
            True or False
        """

        # 处理注册app请求,建立人脸注册通道
        if msg_name == pb2._REGISTERAPP.full_name:
            ret = self._process_register_app(conn, msg_data)
        # 处理人脸注册响应消息
        elif msg_name == pb2._FACERESULT.full_name:
            ret = self._process_face_result(msg_data)
        # 处理人脸识别请求
        elif msg_name == pb2._FRAMEINFO.full_name:
            ret = self._process_frame_info(conn, msg_data)
        # 处理打开通道请求，建立人脸识别通道
        elif msg_name == presenter_message_pb2._OPENCHANNELREQUEST.full_name:
            ret = self._process_open_channel(conn, msg_data)
        # 处理心跳请求，用于保持通道活跃
        elif msg_name == presenter_message_pb2._HEARTBEATMESSAGE.full_name:
            ret = self._process_heartbeat(conn)
        else:
            logging.error("无法识别的消息类型:%s", msg_name)
            ret = False

        return ret

    def _process_register_app(self, conn, msg_data):
        """
        Description: 处理注册app的请求消息，即创建人脸注册通道
        Input:
            conn: 套接字连接
            msg_data: 消息体，由protobuf序列化
        Returns: True or False
        """
        request = pb2.RegisterApp()
        response = pb2.CommonResponse()
        msg_name = pb2._COMMONRESPONSE.full_name
        if not _parse_protobuf(request, msg_data):
            response.ret = pb2.kErrorOther
            response.message = "注册app消息的protobuf解析失败"
            self.send_message(conn, response, msg_name)
            return False

        app_id = request.id
        app_type = request.type

        # 检查app_id是否已存在
        if self.app_manager.is_app_exist(app_id):
            logging.error("app %s已经存在.", app_id)
            response.ret = pb2.kErrorAppRegisterExist
            response.message = "app {} 已存在.".format(app_id)
            self.send_message(conn, response, msg_name)
        elif self.app_manager.get_app_num() >= MAX_APP_NUM:
            logging.error("只允许有一个视频应用")
            response.ret = pb2.kErrorAppRegisterLimit
            response.message = "只允许有一个视频应用"
            self.send_message(conn, response, msg_name)
        else:  # 开始注册app
            self.app_manager.register_app(app_id, conn)
            response.ret = pb2.kErrorNone
            response.message = "注册app {}成功".format(app_id)
            self.send_message(conn, response, msg_name)

            # 注册设备，检查数据库中是否存在对应设备，没有则添加，有则修改状态为运行
            self.app_dict[app_id] = self.mysqlHandle.register_app(app_id)
            self.device_id = self.app_dict[app_id]

            # 初始化这台设备的人脸信息库
            self.registered_faces = self.mysqlHandle.get_all_face_feature(self.device_id)
            return True

        return False

    def _process_face_result(self, msg_data):
        """
        Description: 处理人脸注册响应消息
        Input:
            msg_data: 消息体，由protobuf序列化
        Returns: True or False
        """
        face_result = pb2.FaceResult()
        if not _parse_protobuf(face_result, msg_data):
            # protobuf解析失败
            return False

        # 人脸ID即用户ID
        face_id = face_result.id
        if not self.register_dict.get(face_id):
            # 人脸ID已被删除，不进行注册处理
            logging.warning("人脸ID %s 已被删除", face_id)
            return True

        ret = face_result.response.ret
        if ret != pb2.kErrorNone:
            err_msg = face_result.response.message
            logging.error("获取人脸特征错误信息: %s", err_msg)
            status = FACE_REGISTER_STATUS_FAILED
            message = "获取注册照片的人脸特征失败"
            self._update_register_dict(face_id, status, message)
            return True

        face_num = len(face_result.feature)
        if face_num == 0:
            status = FACE_REGISTER_STATUS_FAILED
            message = "注册照片人脸特征提取失败"
            self._update_register_dict(face_id, status, message)
        elif face_num > 1:
            status = FACE_REGISTER_STATUS_FAILED
            message = "注册照片识别到{}个人脸".format(face_num)
            self._update_register_dict(face_id, status, message)
        else:
            box = face_result.feature[0].box
            face_coordinate = [box.lt_x, box.lt_y, box.rb_x, box.rb_x]
            feature_vector = [i for i in face_result.feature[0].vector]
            if len(feature_vector) != FEATURE_VECTOR_LENGTH:
                logging.error("人脸特征向量维度不是1024")
                status = FACE_REGISTER_STATUS_FAILED
                message = "人脸特征维度不是1024"
                self._update_register_dict(face_id, status, message)
                return True
            return self._save_face_feature(face_id, face_coordinate, feature_vector)
        return True

    def _save_face_feature(self, face_id, face_coordinate, feature_vector):
        """
        Description: 保存人脸特征
        Input:
            face_id: 人脸id，即用户id
            face_coordinate: 人脸框坐标
            feature_vector: 人脸特征向量
        Returns: True or False
        """
        with self.face_lock:
            # 添加人脸特征信息到MySQL数据库
            flag = self.mysqlHandle.add_face_feature(face_id, face_coordinate, feature_vector)
            if flag:
                status = FACE_REGISTER_STATUS_SUCCEED
                message = "人脸信息录入成功"
                self._update_register_dict(face_id, status, message)
                # 更新本地的人脸信息库
                # self.registered_faces = self.mysqlHandle.get_all_face_feature(int(self.device_id))
            else:
                status = FACE_REGISTER_STATUS_FAILED
                message = "用户ID不存在，人脸信息录入失败"
                self._update_register_dict(face_id, status, message)
            return True

    def _update_register_dict(self, face_id, status, message):
        """
        Description: 更新人脸注册字典
        Input:
            face_id: 人脸id
            status: 人脸注册状态
            message: 人脸注册状态信息
        Returns: True or False
        """
        if self.register_dict.get(face_id):
            self.register_dict[face_id]["status"] = status
            self.register_dict[face_id]["message"] = message
            self.register_dict[face_id]["event"].set()

    def _process_frame_info(self, conn, msg_data):
        """
        Description: 处理帧信息消息，即相机实时画面处理
        Input:
            conn: 套接字连接
            msg_data: 消息体，由protobuf序列化
        Returns: True or False
        """
        request = pb2.FrameInfo()
        response = pb2.CommonResponse()
        msg_name = pb2._COMMONRESPONSE.full_name
        if not _parse_protobuf(request, msg_data):
            # protobuf解析失败
            return False

        sock_fileno = conn.fileno()
        handler = self.channel_manager.get_channel_handler_by_fd(sock_fileno)
        if handler is None:
            logging.error("获取通道处理实例失败")
            response.ret = pb2.kErrorOther
            response.message = "获取通道处理实例失败"
            self.send_message(conn, response, msg_name)
            return False

        # 人脸对比
        face_list = self._recognize_face(request.feature, request.temp)
        # 保存帧数据，用于画面显示
        handler.save_frame(request.image, face_list)

        # 响应IO控制消息
        IOCtrl = pb2.CtrlInfo()
        IOCtrl_name = pb2._CTRLINFO.full_name
        if face_list and face_list[0]["is_alive"] and \
                face_list[0]['confidence'] > 0 and request.temp < 37.3:
            # 可以正常通行，添加门禁记录
            IOCtrl.buzzer = 1
            IOCtrl.open = 1
            self.mysqlHandle.add_record(face_list[0]['id'], self.device_id, request.temp)
            self.send_message(conn, IOCtrl, IOCtrl_name)
        elif request.temp >= 37.3:
            # 体温异常，警报
            IOCtrl.buzzer = 0
            IOCtrl.open = 1
            self.send_message(conn, IOCtrl, IOCtrl_name)
            # 若识别成功，则添加异常记录
            if face_list and face_list[0]["is_alive"] \
                and face_list[0]['confidence'] > 0:
                self.mysqlHandle.add_record(face_list[0]['id'], self.device_id, request.temp)
        else:
            # 其他情况
            IOCtrl.buzzer = 1
            IOCtrl.open = 0
            self.send_message(conn, IOCtrl, IOCtrl_name)

        return True

    def _recognize_face(self, face_feature, temp):
        """
        Description:  人脸识别
        Input:
            face_feature: 人脸特征
            temp: 体温值
        Returns: 人脸识别结果
        """
        face_list = []
        for i in face_feature:
            face_info = {}
            box = i.box
            coordinate = [box.lt_x, box.lt_y, box.rb_x, box.rb_y]
            alive_score = i.alive_score
            feature_vector = i.vector
            if len(feature_vector) != FEATURE_VECTOR_LENGTH:
                logging.error("特征向量长度不等于1024")
                continue

            face_info["temp"] = temp
            face_info["coordinate"] = coordinate
            # 判断是否为活体
            if alive_score > 0.9:
                face_info['is_alive'] = True
                # 进行人脸比对
                (face_id, score) = self._compute_face_feature(feature_vector)
                if face_id == -1 or score <= 0:
                    face_info["name"] = "未知"
                else:
                    face_info["id"] = face_id
                    face_info["name"] = self.registered_faces[face_id]["name"]
                face_info["confidence"] = score
            else:
                face_info['is_alive'] = False
                face_info['name'] = '非活体'
                face_info['confidence'] = -1
            face_list.append(face_info)

        return face_list

    def _compute_face_feature(self, feature_vector):
        """
        Description: 计算特征向量对比的分数
        Input:
            feature_vector: 人脸特征向量
        Returns: 人脸ID和分数
        """
        highest_score_face_id = -1
        highest_score = -1
        with self.face_lock:
            # 遍历人脸库，进行对比
            for i in self.registered_faces:
                feature = self.registered_faces[i]["feature"]
                score = _compute_similar_degree(feature, feature_vector)
                if score < self.face_match_threshold:
                    # 小于人脸识别阈值，跳过
                    continue
                if score > highest_score:
                    # 大于最高分数，则更新最高分数，并更新人脸id
                    highest_score = score
                    highest_score_face_id = i

        return highest_score_face_id, highest_score

    def _process_open_channel(self, conn, msg_data):
        """
        Description: 处理打开通道消息，创建人脸识别通道，传输视频帧数据
        Input:
            conn: 套接字连接
            msg_data: 消息体，由protobuf序列化
        Returns: True or False
        """
        request = presenter_message_pb2.OpenChannelRequest()
        response = presenter_message_pb2.OpenChannelResponse()
        if not _parse_protobuf(request, msg_data):
            # protobuf解析失败，返回错误响应
            channel_name = "未知通道"
            err_code = presenter_message_pb2.kOpenChannelErrorOther
            return self._response_open_channel(conn, channel_name,
                                               response, err_code)
        channel_name = request.channel_name

        # 检查通道名称是否存在
        if not self.channel_manager.is_channel_exist(channel_name):
            logging.error("通道名称 %s 不存在.", channel_name)
            err_code = presenter_message_pb2.kOpenChannelErrorNoSuchChannel
            return self._response_open_channel(conn, channel_name,
                                               response, err_code)

        if self.channel_manager.is_channel_busy(channel_name):
            logging.error("通道路径 %s 正忙。", channel_name)
            err = presenter_message_pb2.kOpenChannelErrorChannelAlreadyOpened
            return self._response_open_channel(conn, channel_name,
                                               response, err)

        # 类型为Video,建立视频传输通道
        content_type = presenter_message_pb2.kChannelContentTypeVideo
        if request.content_type == content_type:
            media_type = "video"
        else:
            logging.error("无法识别媒体类型 %s。", request.content_type)
            err_code = presenter_message_pb2.kOpenChannelErrorOther
            return self._response_open_channel(conn, channel_name, response, err_code)

        # 人脸识别通道处理，创建通道资源，并打开通道
        handler = FacialRecognitionHandler(channel_name, media_type)
        sock = conn.fileno()
        self.channel_manager.create_channel_resource(channel_name, sock, media_type, handler)
        err_code = presenter_message_pb2.kOpenChannelErrorNone
        return self._response_open_channel(conn, channel_name, response, err_code)

    def _process_heartbeat(self, conn):
        """
        设置心跳
        Input:
            conn: 套接字连接
        Returns:
            True
        """
        sock_fileno = conn.fileno()
        if self.app_manager.get_app_id_by_socket(sock_fileno):
            # 人脸注册通道发送心跳包
            self.app_manager.set_heartbeat(sock_fileno)

        handler = self.channel_manager.get_channel_handler_by_fd(sock_fileno)
        if handler is not None:
            # 人脸识别通道发送心跳包
            handler.set_heartbeat()
        return True

    def list_registered_apps(self):
        """
        Description:获取已注册的应用程序列表。
        Returns: 应用列表
        """
        return self.app_manager.list_app()

    def get_all_face(self):
        """
        Description: 获取所有已注册的人脸信息。
        """
        with self.face_lock:
            return [i for i in self.registered_faces]

    def save_face_image(self, face_id, image):
        """
        Description: 保存人脸图像。
        Input:
            face_id: 人脸ID,即用户id
            image: 人脸图片
        Returns: True or False
        """
        face_id = str(face_id)
        image_file = os.path.join(self.storage_dir, face_id + ".jpg")
        try:
            # image = image.decode("utf-8")
            # 将人脸照片数据写入本地文件中
            with open(image_file, "wb") as f:
                f.write(image)

            # 向MySQL数据库中添加人脸照片静态路径
            image_url = "http://192.168.2.233:8445/image/jpg/" + face_id + ".jpg"
            self.mysqlHandle.add_face_image(face_id, image_url)
            return True
        except (OSError, TypeError) as exp:
            logging.error(exp)
            return False

    def delete_faces(self, id_list):
        """
        Description: 删除id_list中已注册的人脸信息
        Input:
            id_list:用户id列表
        Returns: True or False
        """
        with self.face_lock:
            for i in id_list:
                i = int(i)
                if self.registered_faces.get(i):
                    backup = self.registered_faces[i]
                    # 删除本地人脸信息库中相关信息
                    del self.registered_faces[i]
                    try:
                        j = str(i)
                        image_file = os.path.join(
                            self.storage_dir, j + ".jpg")
                        # 移除本地保存的人脸文件
                        os.remove(image_file)
                        # 删除数据库中该用户的人脸信息
                        self.mysqlHandle.delete_face(i)
                    except (OSError, JSONDecodeError) as exp:
                        logging.error(exp)
                        self.registered_faces[i] = backup
                        return False
        return True

    def get_app_socket(self, app_id):
        """
        Description: 获取绑定到应用程序的套接字，即返回人脸注册通道的Socket
        Input:
            app_id：应用的id
        Returns: 套接字
        """
        return self.app_manager.get_socket_by_app_id(app_id)

    def stop_thread(self):
        """
        Description:进程退出时清理线程。
        """
        channel_manager = ChannelManager([])
        channel_manager.close_all_thread()
        self.set_exit_switch()
        self.app_manager.set_thread_switch()


class FacialRecognitionManager:
    """人脸识别管理类，向Http服务器提供API接口"""

    __instance = None
    # 人脸识别处理类实例
    server = None

    def __init__(self, server=None):
        """初始化函数"""

    def __new__(cls, server=None):
        """确保单实例. """
        if cls.__instance is None:
            cls.__instance = object.__new__(cls)
            cls.server = server
        return cls.__instance

    def get_app_list(self):
        """
        Description: 获取在线app列表的API
        Returns: app列表
        """
        return self.server.list_registered_apps()

    def get_all_face_id(self):
        """
        Description: 获取所有已注册人脸ID的API
        Returns: a id list
        """
        return self.server.get_all_face()

    def get_faces(self, id_list):
        """
        Description: 用于获取指定人脸信息的API。
        Input: a id list.
        Returns: 列表包括用户ID、用户名称和图片。
        """
        if not isinstance(id_list, list):
            return []

        face_list = []
        for i in id_list:
            face_info = {"id": i, "name": self.server.registered_faces[i]['name']}
            try:
                i = str(i)
                image_file = os.path.join(self.server.storage_dir, i + ".jpg")
                face_info["image"] = open(image_file, 'rb').read()
            except OSError as exp:
                logging.error(exp)
                continue
            face_list.append(face_info)

        return face_list

    def _choose_random_app(self):
        """
        Description: 随机选择一个在线app。
        Returns: a app id
        """
        app_list = self.get_app_list()
        if app_list:
            index = random.randint(0, len(app_list) - 1)
            return app_list[index]
        return None

    # 注册人脸信息，根据用户唯一标识id
    def register_face(self, face_id, image):
        """
        Description: 人脸注册API
        Input:
            face_id: 用户ID
            image: 一张人脸脸图片
        Returns: (ret, msg)
        """
        # 输入参数检查
        if not isinstance(face_id, str):
            return False, "ID不是字符串格式"

        if not isinstance(image, bytes):
            return False, "图片不是字节流格式"

        if self._get_face_number() >= self.server.max_face_num:
            return False, "人脸数量限制"

        app_id = self._choose_random_app()
        if app_id is None:
            return False, "没有正在运行的设备"

        conn = self.server.get_app_socket(app_id)
        if conn is None:
            return False, "网络错误，与设备的Socket通信中断"

        # 向人脸注册通道发送人脸注册请求
        request = pb2.FaceInfo()
        request.id = face_id
        request.image = image

        # 添加注册字典信息
        register_dict = self.server.register_dict
        register_dict[face_id] = {
            "status": FACE_REGISTER_STATUS_WAITING,
            "message": "",
            "event": threading.Event()  # 线程时间
        }

        # 发送注册请求
        msg_name = pb2._FACEINFO.full_name
        self.server.send_message(conn, request, msg_name)

        # 线程时间等待，通过Event().set激活，或等待超时后响应
        register_dict[face_id]["event"].wait(FACE_REGISTER_TIME_OUT)
        # 超时
        if register_dict[face_id]["status"] == FACE_REGISTER_STATUS_WAITING:
            logging.warning("注册人脸%s超时", face_id)
            del register_dict[face_id]
            return False, "10s 超时"
        # 注册失败
        if register_dict[face_id]["status"] == FACE_REGISTER_STATUS_FAILED:
            err_msg = register_dict[face_id]["message"]
            logging.error("注册人脸 %s 失败, 原因是:%s", face_id, register_dict[face_id]["message"])
            del register_dict[face_id]
            return False, err_msg
        # 注册成功,保存人脸照片
        ret = self.server.save_face_image(face_id, image)
        del register_dict[face_id]
        if ret:
            logging.info("注册人脸 %s 成功", face_id)
            return True, "录入人脸成功"
        logging.error("将人脸%s保存到数据库失败", face_id)
        return False, "保存人脸信息失败!"

    def unregister_face(self, id_list):
        """
        Description: 用于注销人脸的 API
        Input:
            name_list: 将被删除的ID列表。
        Returns: True or False
        """
        if isinstance(id_list, list):
            return self.server.delete_faces(id_list)
        logging.error("注销人脸失败")
        return False

    def _get_face_number(self):
        """
        Description: 获取人脸总数
        Returns: 人脸总数
        """
        return len(self.get_all_face_id())


def run():
    """人脸识别服务器运行函数"""

    # 读取配置文件
    config = ConfigParser()

    # 配置日志
    log_file_path = os.path.join(ConfigParser.root_path, "config/logging.conf")
    fileConfig(log_file_path)
    logging.getLogger('facial_recognition')

    if not config_verify():
        return None

    server = FacialRecognitionServer(config)
    FacialRecognitionManager(server)
    return server
