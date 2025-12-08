from ..abstractions import UserPeripheral


class SERIAL_LINK(UserPeripheral):
    """
    The register to write your request from the core to the D2D link.
    Be aware which registers you use, depending on the channel configuration.
    """

    _name = "serial_link"
