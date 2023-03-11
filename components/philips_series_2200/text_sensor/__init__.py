import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from .. import CONTROLLER_ID, PhilipsSeries2200, philips_series_2200_ns


philips_status_sensor_ns = philips_series_2200_ns.namespace("philips_status_sensor")
StatusSensor = philips_status_sensor_ns.class_(
    "StatusSensor", text_sensor.TextSensor, cg.Component
)

CONF_STATUS_TYPE = "for"

StatusType = philips_status_sensor_ns.enum("StatusType")
STATUS_TYPES = {
    "overall": StatusType.OVERALL,
    "led_espresso": StatusType.LED_ESPRESSO,
    "led_hot_water": StatusType.LED_HOT_WATER,
    "led_coffee": StatusType.LED_COFFEE,
    "led_cappuccino": StatusType.LED_CAPPUCCINO,
    "led_beans": StatusType.LED_BEANS,
    "led_size": StatusType.LED_SIZE,
    "led_powder": StatusType.LED_POWDER,
    "led_water_empty": StatusType.LED_WATER_EMPTY,
    "led_waste": StatusType.LED_WASTE,
    "led_start_stop": StatusType.LED_START_STOP
}

CONFIG_SCHEMA = text_sensor.TEXT_SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(StatusSensor),
        cv.Required(CONTROLLER_ID): cv.use_id(PhilipsSeries2200),
        cv.Required(CONF_STATUS_TYPE): cv.enum(STATUS_TYPES, lower=True, space="_"),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)

    cg.add(parent.add_status_sensor(var))
