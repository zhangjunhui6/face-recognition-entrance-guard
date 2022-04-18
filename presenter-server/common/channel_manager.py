"""presenter channel manager module"""

import logging
import threading

# max support 10 channels
MAX_CHANNEL_NUM = 10

# when a channel have receive data,
# the active status will last 3 seconds
ACTIVE_LAST_TIME = 3


class ChannelResource:
    """every channel has a  ChannelResource object, contains a ChannelHandler object
    and a socket fileno. it corresponding to the ChannelFd one by one
    """
    def __init__(self, handler, socket=None):
        self.handler = handler
        self.socket = socket


class ChannelFd:
    """every channel has a  ChannelFd object, contains a ChannelHandler
    object and channel name. It corresponds to the ChannelResource one by one
    """
    def __init__(self, channel_name, handler):
        self.channel_name = channel_name
        self.handler = handler


class Channel:
    """record user register channels
        self.image: if channel type is image, save the image here
    """
    def __init__(self, channel_name):
        self.channel_name = channel_name
        self.image = None
        self.rectangle_list = None


class ChannelManager:
    """manage all the api about channel
    __instance: ensure it is a single instance
    _channel_resources: a dict
        key: channel name
        value: a ChannelResource() object.
    _channel_fds: a dict
        key: socket fileno
        value: a ChannelFd() object.
    _channel_list: a list, member is a Channel() object."""

    __instance = None
    channel_resources = {}
    channel_resource_lock = threading.Lock()
    channel_fds = {}
    channel_fds_lock = threading.Lock()
    channel_list = []
    channel_lock = threading.Lock()

    err_code_ok = 0
    err_code_too_many_channel = 1
    err_code_repeat_channel = 2

    def __init__(self, channel_list=None):
        """init func"""

    def __new__(cls, channel_list=None):
        """ensure only a single instance created. """
        if cls.__instance is None:
            cls.__instance = object.__new__(cls)
        return cls.__instance

    # Register one channel
    def register_one_channel(self, channel_name):
        """
        register a channel path, user create a channel via browser
        """
        with self.channel_lock:
            if len(self.channel_list) >= MAX_CHANNEL_NUM:
                logging.info("register channel: %s fail, \
                                exceed max number 10.", channel_name)
                return self.err_code_too_many_channel
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    logging.info("register channel: %s fail, \
                                    already exist.", channel_name)
                    return self.err_code_repeat_channel

            self.channel_list.append(Channel(channel_name=channel_name))
            logging.info("register channel: %s", channel_name)
            return self.err_code_ok

    # Unregister one channel
    def unregister_one_channel(self, channel_name):
        """
        unregister a channel path, user delete a channel via browser
        """
        with self.channel_lock:
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    self.clean_channel_resource_by_name(channel_name)
                    logging.info("unregister channel: %s", channel_name)
                    del self.channel_list[i]
                    break

    # Determine whether the channel exists
    def is_channel_exist(self, channel_name):
        """
        Check if a channel is exist
        True: exist
        False: not exist
        """
        with self.channel_lock:
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    return True
            return False

    # Create channel resource
    def _register_channel_fd(self, sock_fileno, channel_name):
        """Internal func, create a ChannelFd object"""
        if self.channel_fds.get(sock_fileno):
            del self.channel_fds[sock_fileno]
        handler = self.channel_resources[channel_name].handler
        self.channel_fds[sock_fileno] = ChannelFd(channel_name, handler)

    def create_channel_resource(self, channel_name, channel_fd, media_type, handler):
        """create a ChannelResource object which contains all the resources
           binding a channel.
        channel_name: channel name.
        channel_fd: socket fileno binding the channel.
        media_type: support image or video.(useless)
        handler: an channel handler process image data.
        """
        with self.channel_resource_lock:
            log_info = "create channel resource,"
            log_info += " channel_name:%s, channel_fd:%u, media_type:%s"
            logging.info(log_info, channel_name, channel_fd, media_type)
            self.channel_resources[channel_name] = \
                ChannelResource(handler=handler, socket=channel_fd)
            self._register_channel_fd(channel_fd, channel_name)

    # Clean channel resource
    def _clean_channel_resource(self, channel_name):
        """Internal func, clean channel resource by channel name"""
        if self.channel_resources.get(channel_name):
            self.channel_resources[channel_name].handler.close_thread()
            self.channel_resources[channel_name].handler.web_event.set()
            self.channel_resources[channel_name].handler.image_event.set()
            del self.channel_resources[channel_name]
            logging.info("clean channel: %s's resource", channel_name)

    def clean_channel_resource_by_fd(self, sock_fileno):
        """
        clean channel resource by socket fileno
        sock_fileno: socket fileno which binding to an channel
        """
        with self.channel_fds_lock:
            with self.channel_resource_lock:
                if self.channel_fds.get(sock_fileno):
                    self._clean_channel_resource(self.channel_fds[sock_fileno].channel_name)
                    del self.channel_fds[sock_fileno]

    def clean_channel_resource_by_name(self, channel_name):
        """clean channel resource by channel_name
        channel_name: channel name"""
        if self.channel_resources.get(channel_name):
            self.clean_channel_resource_by_fd(self.channel_resources[channel_name].socket)

    # Get channel handler
    def get_channel_handler_by_fd(self, sock_fileno):
        """get channel handler by socket fileno"""
        with self.channel_fds_lock:
            if self.channel_fds.get(sock_fileno):
                return self.channel_fds[sock_fileno].handler
            return None

    def get_channel_handler_by_name(self, channel_name):
        """
        get the channel handler by channel name
        """
        with self.channel_resource_lock:
            if self.channel_resources.get(channel_name):
                return self.channel_resources[channel_name].handler
            return None

    # check channel resource whether exist
    def is_channel_busy(self, channel_name):
        """check if channel is busy """
        with self.channel_resource_lock:
            if self.channel_resources.get(channel_name):
                return True
            return False

    # Call the close_thread of the handle
    def close_all_thread(self):
        """if a channel process video type, it will create a thread.
        this func can close the thread.
        """
        with self.channel_resource_lock:
            for channel_name in self.channel_resources:
                self.channel_resources[channel_name].handler.close_thread()

    # List channels
    def list_channels(self):
        """
        return all the channel name and the status
        status is indicating active state or not
        """
        with self.channel_lock:
            return [{'status': self.is_channel_busy(i.channel_name),
                     'name': i.channel_name} for i in self.channel_list]

    # channel image process
    def save_channel_image(self, channel_name, image_data, rectangle_list):
        """
        when a channel bounding to image type,
        server will permanent hold an image for it.
        this func save a image in memory
        """
        with self.channel_lock:
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    self.channel_list[i].image = image_data
                    self.channel_list[i].rectangle_list = rectangle_list
                    break

    def get_channel_image(self, channel_name):
        """
        when a channel bounding to image type,
        server will permanent hold an image for it.
        this func get the image
        """
        with self.channel_lock:
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    return self.channel_list[i].image

            # channel not exist
            return None

    def get_channel_image_with_rectangle(self, channel_name):
        """
        A new method for display server,
        return the image and rectangle list
        """
        with self.channel_lock:
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    return self.channel_list[i].image, self.channel_list[i].rectangle_list
            return None, None

    def clean_channel_image(self, channel_name):
        """
        when a channel bounding to image type,
        server will permanent hold an image for it.
        this func clean the image
        """
        with self.channel_lock:
            for i in range(len(self.channel_list)):
                if self.channel_list[i].channel_name == channel_name:
                    self.channel_list[i].image = None
                    break
