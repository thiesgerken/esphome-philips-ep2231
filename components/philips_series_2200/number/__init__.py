import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from .. import CONTROLLER_ID, PhilipsSeries2200, philips_series_2200_ns

DEPENDENCIES = ["philips_series_2200"]

CONF_TYPE = "type"

philips_beverage_setting_ns = philips_series_2200_ns.namespace(
    "philips_beverage_setting"
)
BeverageSetting = philips_beverage_setting_ns.class_(
    "BeverageSetting", number.Number, cg.Component
)

Type = philips_beverage_setting_ns.enum("Type")
TYPES = {
    "beans": Type.BEAN,
    "size": Type.SIZE,
}

CONFIG_SCHEMA = (
    number.number_schema(BeverageSetting)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(BeverageSetting),
            cv.Required(CONTROLLER_ID): cv.use_id(PhilipsSeries2200),
            cv.Required(CONF_TYPE): cv.enum(TYPES, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(var, config, min_value=1, max_value=3, step=1)

    cg.add(var.set_type(config[CONF_TYPE]))
    cg.add(parent.add_beverage_setting(var))
