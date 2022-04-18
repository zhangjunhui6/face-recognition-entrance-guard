from datetime import datetime

import pymysql
import json


class MysqlHandle:
    """ MySQL数据库处理类，提供数据库操作API"""

    def __init__(self, host='127.0.0.1', port=3306, user='root',
                 password='zjh1314520', db='AcsCtl'):
        """ 初始化方法 """
        # 建立MySQL连接
        self.conn = pymysql.connect(host=host, port=port, user=user,
                                    password=password, db=db)
        # 创建游标
        self.cur = self.conn.cursor()

    def __exit__(self, exc_type, exc_val, exc_tb):
        """ 退出时执行的方法 """
        # 关闭游标
        self.cur.close()
        # 关闭数据库连接
        self.conn.close()

    def register_app(self, app_id):
        """
        Description: 注册app/设备，查询设备是否已注册，注册并改变状态为run
        :param app_id 设备名称
        :return device_id 设备ID
        """
        # 查询设备是否存在
        sql = "select * from device where name = '%s'" % app_id
        self.cur.execute(sql)
        self.conn.commit()
        # 不存在则注册
        if self.cur.rowcount == 0:
            sql = "insert into device(name, state) value ('%s', 'run')" % app_id
            try:
                self.cur.execute(sql)
                self.conn.commit()
            except:
                self.conn.rollback()
        else:
            # 存在则改变状态
            id = self.cur.fetchone()[0]
            sql = "update device set state = 'run' where id = '%d' " % id
            try:
                self.cur.execute(sql)
                self.conn.commit()
            except:
                self.conn.rollback()
            return id

        # 返回device_id
        sql = "select * from device where name = '%s' " % app_id
        self.cur.execute(sql)
        self.conn.commit()
        return self.cur.fetchone()[0]

    def unregister_app(self, device_id):
        """注销设备/app,改变设备状态(stop)"""
        sql = "update device set state = 'stop' where id = '%s' " % device_id
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()

    def get_all_face_feature(self, device_id=3):
        """获取某设备能够识别的所有人脸信息"""
        sql = "select id, name, facefeature from user where id in " \
              "(select uid from user_device where did = '%s')" % device_id
        result = {}
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()

        results = self.cur.fetchall()
        for row in results:
            if row[2] is None:
                continue
            id = row[0]
            feature = json.loads(row[2])
            feature['name'] = row[1]
            result[id] = feature
        return result

    def add_face_feature(self, user_id, face_coordinate, face_feature):
        """录入人脸信息"""
        sql = "select * from user where id = '%s'" % user_id
        self.cur.execute(sql)
        self.conn.commit()
        if self.cur.rowcount == 0:
            return False
        json_data = json.dumps({"coordinate": face_coordinate, "feature": face_feature})
        sql = "update user set facefeature = '" + json_data + "' where id = '%s'" % user_id
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()
        return True

    def add_face_image(self, user_id, image):
        """添加人脸照片信息"""
        sql = "update user set image = '%s' where id = '%s'" % (image, user_id)
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()

    def get_face_image(self, user_id):
        """ 获取照片信息"""
        sql = "select image from user where id = '%s'" % user_id
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()
        if self.cur.rowcount == 1:
            image = self.cur.fetchone()[0]
            if image is not None:
                return True, image
        return False, ""

    def delete_face(self, user_id):
        """删除人脸信息"""
        sql = "update user set facefeature = null,image = null where id = '%s' " % user_id
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()

    def add_record(self, uid, did, temp):
        """添加门禁记录，如果间隔时间少于3s则为上一个识别的人，则不添加记录。"""
        sql = "select uid,did,time from records where id in (select max(id) from records);"
        dt = datetime.now()
        self.cur.execute(sql)
        self.conn.commit()
        if self.cur.rowcount == 1:
            arr = self.cur.fetchone()
            if arr[0] == uid and arr[1] == did and (dt-arr[2]).seconds <= 3:
                return None

        dt = dt.strftime("%Y-%m-%d %H:%M:%S")
        sql = "insert into records(uid,did,temp,time) values(%d, %d, %.2f, '%s');" % (uid, did, temp, dt)
        try:
            self.cur.execute(sql)
            self.conn.commit()
        except:
            self.conn.rollback()


if __name__ == '__main__':
    """测试功能"""
    mysql = MysqlHandle()
    ret = mysql.get_all_face_feature()
    print(ret)