# autogenerated by genmsg_py from CartesianArmServerFeedback.msg. Do not edit.
import roslib.message
import struct

import geometry_msgs.msg

class CartesianArmServerFeedback(roslib.message.Message):
  _md5sum = "02a6b904b8484b60a59895e6158270a9"
  _type = "cart_interp/CartesianArmServerFeedback"
  _has_header = False #flag to mark the presence of a Header object
  _full_text = """# ====== DO NOT MODIFY! AUTOGENERATED FROM AN ACTION DEFINITION ======
# feedback definition
geometry_msgs/Pose currentpoint

================================================================================
MSG: geometry_msgs/Pose
# A representation of pose in free space, composed of postion and orientation. 
Point position
Quaternion orientation

================================================================================
MSG: geometry_msgs/Point
# This contains the position of a point in free space
float64 x
float64 y
float64 z

================================================================================
MSG: geometry_msgs/Quaternion
# This represents an orientation in free space in quaternion form.

float64 x
float64 y
float64 z
float64 w

"""
  __slots__ = ['currentpoint']
  _slot_types = ['geometry_msgs/Pose']

  ## Constructor. Any message fields that are implicitly/explicitly
  ## set to None will be assigned a default value. The recommend
  ## use is keyword arguments as this is more robust to future message
  ## changes.  You cannot mix in-order arguments and keyword arguments.
  ##
  ## The available fields are:
  ##   currentpoint
  ##
  ## @param self: self
  ## @param args: complete set of field values, in .msg order
  ## @param kwds: use keyword arguments corresponding to message field names
  ## to set specific fields. 
  def __init__(self, *args, **kwds):
    if args or kwds:
      super(CartesianArmServerFeedback, self).__init__(*args, **kwds)
      #message fields cannot be None, assign default values for those that are
      if self.currentpoint is None:
        self.currentpoint = geometry_msgs.msg.Pose()
    else:
      self.currentpoint = geometry_msgs.msg.Pose()

  ## internal API method
  def _get_types(self): return self._slot_types

  ## serialize message into buffer
  ## @param buff StringIO: buffer
  def serialize(self, buff):
    try:
      buff.write(struct.pack('<7d', self.currentpoint.position.x, self.currentpoint.position.y, self.currentpoint.position.z, self.currentpoint.orientation.x, self.currentpoint.orientation.y, self.currentpoint.orientation.z, self.currentpoint.orientation.w))
    except struct.error, se: self._check_types(se)
    except TypeError, te: self._check_types(te)

  ## unpack serialized message in str into this message instance
  ## @param self: self
  ## @param str str: byte array of serialized message
  def deserialize(self, str):
    try:
      if self.currentpoint is None:
        self.currentpoint = geometry_msgs.msg.Pose()
      end = 0
      start = end
      end += 56
      (self.currentpoint.position.x, self.currentpoint.position.y, self.currentpoint.position.z, self.currentpoint.orientation.x, self.currentpoint.orientation.y, self.currentpoint.orientation.z, self.currentpoint.orientation.w,) = struct.unpack('<7d',str[start:end])
      return self
    except struct.error, e:
      raise roslib.message.DeserializationError(e) #most likely buffer underfill


  ## serialize message with numpy array types into buffer
  ## @param self: self
  ## @param buff StringIO: buffer
  ## @param numpy module: numpy python module
  def serialize_numpy(self, buff, numpy):
    try:
      buff.write(struct.pack('<7d', self.currentpoint.position.x, self.currentpoint.position.y, self.currentpoint.position.z, self.currentpoint.orientation.x, self.currentpoint.orientation.y, self.currentpoint.orientation.z, self.currentpoint.orientation.w))
    except struct.error, se: self._check_types(se)
    except TypeError, te: self._check_types(te)

  ## unpack serialized message in str into this message instance using numpy for array types
  ## @param self: self
  ## @param str str: byte array of serialized message
  ## @param numpy module: numpy python module
  def deserialize_numpy(self, str, numpy):
    try:
      if self.currentpoint is None:
        self.currentpoint = geometry_msgs.msg.Pose()
      end = 0
      start = end
      end += 56
      (self.currentpoint.position.x, self.currentpoint.position.y, self.currentpoint.position.z, self.currentpoint.orientation.x, self.currentpoint.orientation.y, self.currentpoint.orientation.z, self.currentpoint.orientation.w,) = struct.unpack('<7d',str[start:end])
      return self
    except struct.error, e:
      raise roslib.message.DeserializationError(e) #most likely buffer underfill

