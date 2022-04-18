"""presenter channel manager module"""

import time
import logging
import threading
from threading import get_ident
from common.channel_manager import ChannelManager

# thread event timeout, The unit is second.
WEB_EVENT_TIMEOUT = 2

IMAGE_EVENT_TIMEOUT = 10

# heart beat timeout.
HEARTBEAT_TIMEOUT = 100


class ThreadEvent:
    """
    An Event-like class that signals all active clients when a new frame is available.
    """
    def __init__(self, timeout=None):
        self.events = {}
        self.timeout = timeout

    # 一个线程标识一个事件，获取当前线程的事件，事件等待使线程处于阻塞状态
    def wait(self):
        """Invoked from each client's thread to wait for the next frame."""
        ident = get_ident()  # 线程标识符
        if ident not in self.events:
            # this is a new client
            self.events[ident] = [threading.Event(), time.time()]
        return self.events[ident][0].wait(self.timeout)

    # 唤醒所有被阻塞的线程，同时移除运行5s以上的线程
    def set(self):
        """Invoked by the camera thread when a new frame is available."""
        now = time.time()
        remove = None
        for ident, event in self.events.items():
            if not event[0].isSet():
                # if this client's event is not set, then set it
                # also update the last set timestamp to now
                event[0].set()
                event[1] = now
            else:
                if now - event[1] > 5:
                    remove = ident
        if remove:
            del self.events[remove]

    # 将event的标志设置为False,阻塞所有调用wait方法的线程
    def clear(self):
        """Invoked from each client's thread after a frame was processed."""
        self.events[get_ident()][0].clear()


class ChannelHandler:
    """A set of channel handlers, process data received from channel"""
    def __init__(self, channel_name, media_type):
        self.channel_name = channel_name
        self.media_type = media_type
        # last time the channel receive data.
        self._frame = None

        self.thread = None
        self.lock = threading.Lock()
        self.heartbeat = time.time()
        self.web_event = ThreadEvent(timeout=WEB_EVENT_TIMEOUT)
        self.image_event = ThreadEvent(timeout=IMAGE_EVENT_TIMEOUT)
        self.channel_manager = ChannelManager([])
        # image
        self.img_data = None
        self.width = None
        self.height = None
        self.rectangle_list = None

        if media_type == "video":
            self.thread_name = "videothread-{}".format(self.channel_name)
            self.heartbeat = time.time()
            self.close_thread_switch = False
            self.fps = 0
            self.image_number = 0
            self.time_list = []
            self._create_thread()

    # create thread
    def _create_thread(self):
        """Start the background video thread if it isn't running yet."""
        if self.thread is not None and self.thread.is_alive():
            return

        # start background frame thread
        self.thread = threading.Thread(target=self._video_thread)
        self.thread.start()

    # video thread
    def _video_thread(self):
        """background thread to process video"""
        logging.info('create %s...', self.thread_name)
        for frame in self.frames():
            if frame:
                # send signal to clients
                self._frame = frame
                self.web_event.set()

            # exit thread
            if self.close_thread_switch:
                self.channel_manager.clean_channel_resource_by_name(self.channel_name)
                logging.info('Stop thread:%s.', self.thread_name)
                break

    def frames(self):
        """a generator generates image"""
        while True:
            self.image_event.wait()
            self.image_event.clear()
            if self.img_data:
                yield self.img_data
                self.img_data = None

            # if set _close_thread_switch, return immediately
            if self.close_thread_switch:
                yield None

            # if no frames or heartbeat coming in the last 100 seconds,
            # stop the thread and close socket
            if time.time() - self.heartbeat > HEARTBEAT_TIMEOUT:
                self.set_thread_switch()
                self.img_data = None
                yield None

    def get_media_type(self):
        """get media_type, support image or video"""
        return self.media_type

    def set_heartbeat(self):
        """record heartbeat"""
        self.heartbeat = time.time()

    def set_thread_switch(self):
        """record heartbeat"""
        self.close_thread_switch = True

    def save_image(self, data, width, height, rectangle_list):
        """save image receive from socket"""
        self.width = width
        self.height = height
        self.rectangle_list = rectangle_list

        # compute fps if type is video
        if self.media_type == "video":
            while self.img_data:
                time.sleep(0.01)

            self.time_list.append(self.heartbeat)
            self.image_number += 1
            while self.time_list[0] + 1 < time.time():
                self.time_list.pop(0)
                self.image_number -= 1
                if self.image_number == 0:
                    break

            self.fps = len(self.time_list)
            self.img_data = data
            self.image_event.set()
        else:
            self.img_data = data
            self.channel_manager.save_channel_image(self.channel_name, self.img_data, self.rectangle_list)

        self.heartbeat = time.time()

    def get_image(self):
        """get image_data"""
        return self.img_data

    def get_frame(self):
        """Return the current video frame."""
        # wait util receive a frame data, and push it to your browser.
        ret = self.web_event.wait()
        self.web_event.clear()
        # True: _web_event return because set()
        # False: _web_event return because timeout
        if ret:
            return self._frame, self.fps, self.width, self.height, self.rectangle_list

        return None, None, None, None, None

    # close thread
    def close_thread(self):
        """close thread if object has created"""
        if self.thread is None:
            return

        self.set_thread_switch()
        self.image_event.set()
        logging.info("%s set _close_thread_switch True", self.thread_name)