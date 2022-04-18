"""
Http服务器
"""
import os
import random
import base64
import threading
import time
import logging

import tornado.httpserver
import tornado.ioloop
import tornado.web
import tornado.gen
import tornado.websocket

import common.channel_manager as channel_manager
import facial_recognition.src.config_parser as config_parser
import facial_recognition.src.facial_recognition_server as facial_recognition_server

# app
G_WEBAPP = None

# jpeg base64 头部
JPEG_BASE64_HEADER = "data:image/jpeg;base64,"

REQUEST = "req"

APP_NAME = "app_name"

USER_ID = "user_id"

IMAGE = "image_data"

# 返回码
RET_CODE_SUCCESS = "0"
RET_CODE_FAIL = "1"
RET_CODE_LOADING = "2"


class WebApp:
    """
    Web应用程序
    """
    __instance = None

    def __init__(self):
        """
        初始化方法
        """
        # 通道管理实例
        self.channel_mgr = channel_manager.ChannelManager()
        # 人脸识别管理实例
        self.facial_recognize_manage = facial_recognition_server.FacialRecognitionManager()

        # 请求列表
        self.request_list = set()
        self.lock = threading.Lock()
        # 视频状态,ret为1表示有设备运行，name为设备名称app_id，req为获取视频帧请求，随机数标识
        self.videoState = {"ret": 0, "msg": {"name": "", "req": 0}}

    def __new__(cls, *args, **kwargs):
        # 单实例创建
        if cls.__instance is None:
            cls.__instance = object.__new__(cls)
        return cls.__instance

    def list_registered_apps(self):
        """获取已注册的应用程序"""
        app_list = self.facial_recognize_manage.get_app_list()
        ret = []
        idx = 1
        for item in app_list:
            ret.append({"id": idx, "appName": item})
            idx = idx + 1

        return ret

    def list_all_face_id(self):
        """列出所有注册人脸的用户ID"""
        return self.facial_recognize_manage.get_all_face_id()

    def list_all_face(self):
        """列出所有注册人脸的用户人脸照片"""
        id_list = self.list_all_face_id()

        if not id_list:
            return []
        id_list = sorted(id_list)
        show_face = self.facial_recognize_manage.get_faces(id_list)
        for item in show_face:
            try:
                # 将二进制数据转换为base64格式
                item["image"] = JPEG_BASE64_HEADER + base64.b64encode(item["image"]).decode("utf-8")
            except (ValueError, TypeError) as exp:
                logging.error(exp)
                return []

        return show_face

    def get_videoState(self):
        """获取视频状态"""
        tmpVideoState = self.videoState
        tmpList = self.facial_recognize_manage.get_app_list()
        if tmpVideoState["ret"] == 1 and tmpVideoState["msg"]["name"] in tmpList:
            # 若想要获取视频的设备正在运行
            return self.videoState
        else:
            tmpVideoState["ret"] = 0
            self.videoState = tmpVideoState
            return tmpVideoState

    def is_channel_exists(self, name):
        """判断人脸注册通道是否存在"""
        return self.channel_mgr.is_channel_exist(name)

    def add_request(self, request):
        """
        新增请求
        @param  request: 需要被存储的请求项
        @note: 各个请求必须都不相同，请求由(app名/通道名，随机数)构成.
        """
        with self.lock:
            self.request_list.add(request)
            self.videoState = {"ret": 1, "msg": {"name": request[1], "req": request[0]}}

    def has_request(self, request):
        """
        是否存在请求request
        @param  request:  被检查的对象.
        @return:  True or False.
        """
        with self.lock:
            for item in self.request_list:
                # 判断请求是否相同
                if item[0] == request[0] and item[1] == request[1]:
                    return True

            return False

    def get_media_data(self, app_name):
        """获取媒体数据"""
        ret = {"ret": RET_CODE_FAIL, "image": "", "fps": "0", "face_list": ""}

        if self.is_channel_exists(app_name) is False:
            return ret

        handler = self.channel_mgr.get_channel_handler_by_name(app_name)

        ret["ret"] = RET_CODE_LOADING
        if handler is not None:
            frame_info = handler.get_frame()
        else:
            return ret

        if not frame_info:
            return ret

        try:
            ret["image"] = base64.b64encode(frame_info["image"]).decode("utf-8")
        except (TypeError, ValueError) as exp:
            logging.error(exp)
            return ret

        ret["ret"] = RET_CODE_SUCCESS
        ret["fps"] = frame_info["fps"]
        ret["face_list"] = frame_info["face_list"]

        return ret

    def register_face(self, user_id, image_data):
        """ 人脸注册"""
        ret = {"ret": RET_CODE_FAIL, "msg": ""}

        if user_id is None:
            logging.info("用户ID不能为空!")
            ret["msg"] = "用户ID不能为空!"
            return ret

        user_id = user_id.strip()

        if user_id == "":
            logging.info("用户ID不能为空!")
            ret["msg"] = "用户ID不能为空!"
            return ret

        if image_data is None:
            logging.info("人脸图片不能为空")
            ret["msg"] = "人脸图片不能为空！"
            return ret

        # 检查图片的base64编码
        if len(image_data) <= len(JPEG_BASE64_HEADER):
            logging.info("只支持 jpg/jpeg 格式图片")
            ret["msg"] = "只支持 jpg/jpeg 格式图片"
            return ret

        if image_data[0:len(JPEG_BASE64_HEADER)] != JPEG_BASE64_HEADER:
            logging.info("只支持 jpg/jpeg 格式图片")
            ret["msg"] = "只支持 jpg/jpeg 格式图片"
            return ret

        # 去除base64头部
        img_data = image_data[len(JPEG_BASE64_HEADER):len(image_data)]

        try:
            # 转换成二进制数据
            decode_img = base64.b64decode(img_data)
        except (ValueError, TypeError) as exp:
            logging.error(exp)
            return {"ret": RET_CODE_FAIL, "msg": "图片解码错误,由base64转2进制失败"}

        # 注册人脸
        flag = self.facial_recognize_manage.register_face(user_id, decode_img)
        if flag[0] is True:
            logging.info("人脸注册成功")
            ret = {"ret": RET_CODE_SUCCESS, "msg": flag[1]}
        else:
            logging.info("人脸注册失败")
            ret = {"ret": RET_CODE_FAIL, "msg": flag[1]}

        return ret

    def unregister_face(self, id_list):
        """删除已注册的人脸信息"""
        ret = {"ret": RET_CODE_FAIL, "msg": ""}

        if not id_list:
            logging.info("ID列表不能为空")
            ret["msg"] = "ID列表不能为空"
            return ret

        flag = self.facial_recognize_manage.unregister_face(id_list)
        if flag is False:
            ret["ret"] = RET_CODE_FAIL
            ret["msg"] = "删除人脸失败"
            logging.info("删除人脸失败")
        elif flag is True:
            ret["ret"] = RET_CODE_SUCCESS
            ret["msg"] = "删除人脸成功"
            logging.info("删除人脸成功")

        return ret


class BaseHandler(tornado.web.RequestHandler):
    """
    基本处理类
    """

    def data_received(self, chunk):
        pass


class AppListHandler(BaseHandler):
    """
    处理AppList请求
    """

    @tornado.gen.coroutine
    def get(self):
        """
        仅处理/home或/index的get请求,传递app列表、所有人脸信息和视频状态信息
        """
        self.render("home.html",
                    listret=(G_WEBAPP.list_registered_apps(), G_WEBAPP.list_all_face(), G_WEBAPP.get_videoState()))


class ViewHandler(BaseHandler):
    """
    处理画面请求
    """

    @tornado.web.asynchronous
    def get(self):
        """
        获取画面频道请求，通过app_name构造请求，并返回请求
        """
        channel_name = self.get_argument(APP_NAME, '')
        if G_WEBAPP.is_channel_exists(channel_name):
            req_id = str(random.random())
            G_WEBAPP.add_request((req_id, channel_name))
            self.finish({"ret": RET_CODE_SUCCESS, "msg": req_id})
        else:
            self.finish({"ret": RET_CODE_FAIL, "msg": "通道不存在"})


class WebSocket(tornado.websocket.WebSocketHandler):
    """
    web socket请求交互
    """

    def data_received(self, chunk):
        pass

    def __init__(self, application, request, **kwargs):
        super().__init__(application, request)
        self.req_id = None
        self.channel_name = None

    def open(self):
        """
        客户端通过wss请求打开websocket通信，接收req_id和通道名
        """
        self.req_id = self.get_argument(REQUEST, '', True)
        self.channel_name = self.get_argument(APP_NAME, '', True)

        # 判断请求是否存在，不存在则关闭连接
        if not G_WEBAPP.has_request((self.req_id, self.channel_name)):
            self.close()

    def on_close(self):
        """
        关闭websocket
        """

    # @tornado.web.asynchronous
    @tornado.gen.coroutine
    def on_message(self, message):
        """
         从客户端接收数据时的响应函数，接收到next，执行run_task函数
        """
        if message == "next":
            self.run_task()

    def run_task(self):
        """
        发送帧数据到客户端
        """

        # 再一次检查通道和请求
        if not G_WEBAPP.is_channel_exists(self.channel_name) or \
                not G_WEBAPP.has_request((self.req_id, self.channel_name)):
            self.close()
            return

        # 获取媒体请求
        result = G_WEBAPP.get_media_data(self.channel_name)
        # 超时响应
        if result['ret'] != RET_CODE_SUCCESS:
            time.sleep(0.1)

        # websocket退出
        if result['ret'] == RET_CODE_FAIL:
            self.close()
        # 成功，返回帧数据
        else:
            ret = WebSocket.send_message(self, result)

    @staticmethod
    def send_message(obj, message, binary=False):
        """
        发送数据到客户端
        """

        # 检查socket通信
        if not obj.ws_connection or not obj.ws_connection.stream.socket:
            return False

        ret = False
        try:
            obj.write_message(message, binary)
            ret = True
        except tornado.websocket.WebSocketClosedError:
            ret = False

        return ret


class RegisterHandler(BaseHandler):
    """
    人脸注册请求处理
    """

    @tornado.web.asynchronous
    def post(self):
        """
        处理人脸注册post请求
        """
        user_id = self.get_argument(USER_ID, '')
        id_list = G_WEBAPP.list_all_face_id()

        # check user name is duplicate
        for item in id_list:
            item = str(item)
            if user_id == item:
                self.finish({"ret": RET_CODE_FAIL, "msg": "用户人脸信息已录入，请勿重复录入"})
                return None

        image_data = self.get_argument(IMAGE, '')
        # 注册人脸
        self.finish(G_WEBAPP.register_face(user_id, image_data))
        return None


class DelFaceHandler(BaseHandler):
    """
    处理删除人脸请求
    """

    @tornado.web.asynchronous
    def post(self):
        """
        删除人脸post请求
        """
        id_list = self.get_arguments(USER_ID)
        self.finish(G_WEBAPP.unregister_face(id_list))


def get_webapp():
    """
   建立web应用
    """
    # 获取模板文件路径和静态文件路径
    templatePath = os.path.join(config_parser.ConfigParser.get_root_path(), "ui/templates")
    staticFilePath = os.path.join(config_parser.ConfigParser.get_root_path(), "ui/static")
    faceImagePath = os.path.join(config_parser.ConfigParser.get_root_path(), "ui/static/face_images")
    # 创建app对象，绑定处理类与路由
    app = tornado.web.Application(handlers=[(r"/", AppListHandler),
                                            (r"/view", ViewHandler),
                                            (r"/websocket", WebSocket),
                                            (r"/register", RegisterHandler),
                                            (r"/delete", DelFaceHandler),
                                            (r"/static/(.*)", tornado.web.StaticFileHandler, {"path": staticFilePath}),
                                            (r"/image/jpg/(.*)", tornado.web.StaticFileHandler, {"path": faceImagePath})
                                            ],
                                  template_path=templatePath)

    # 创建Http服务
    http_server = tornado.httpserver.HTTPServer(app)
    return http_server


def start_webapp():
    """
    启动web应用
    """
    global G_WEBAPP
    G_WEBAPP = WebApp()

    http_server = get_webapp()
    config = config_parser.ConfigParser()
    http_server.listen(config.web_server_port, address=config.web_server_ip)

    print("请点击 http://" + config.web_server_ip + ":" + str(config.web_server_port) + " 访问应用")
    tornado.ioloop.IOLoop.instance().start()


def stop_webapp():
    """
    停止web应用
    """
    tornado.ioloop.IOLoop.instance().stop()
