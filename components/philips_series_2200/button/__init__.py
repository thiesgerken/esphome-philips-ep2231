import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID
from .. import CONTROLLER_ID, PhilipsSeries2200, philips_series_2200_ns

DEPENDENCIES = ["philips_series_2200"]

CONF_ACTION = "action"

philips_action_button_ns = philips_series_2200_ns.namespace("philips_action_button")
ActionButton = philips_action_button_ns.class_("ActionButton", button.Button, cg.Component)

Action = philips_action_button_ns.enum("ActionButton")
ACTIONS = {
    "coffee": Action.COFFEE,
    "espresso": Action.ESPRESSO,
    "hot_water": Action.HOT_WATER,
    "cappuccino": Action.CAPPUCCINO,
    "beans": Action.BEANS,
    "size": Action.SIZE,
    "aqua_clean": Action.AQUA_CLEAN,
    "calc_clean": Action.CALC_CLEAN,
    "start_stop": Action.START_STOP,
}

CONFIG_SCHEMA = button.BUTTON_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ActionButton),
        cv.Required(CONTROLLER_ID): cv.use_id(PhilipsSeries2200),
        cv.Required(CONF_ACTION): cv.enum(ACTIONS, lower=True, space="_"),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    parent = await cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)

    cg.add(var.set_action(config[CONF_ACTION]))
    cg.add(parent.add_action_button(var))
