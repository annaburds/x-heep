from ..abstractions import UserPeripheral


class SERIAL_LINK_REG(UserPeripheral):
    """
    dedicated address space for configuring serial link IP registers.
    """

    _name = "serial_link_reg"
