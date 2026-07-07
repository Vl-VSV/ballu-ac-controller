import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID

AUTO_LOAD = ["climate"]
CODEOWNERS = ["@ballu_ac_controller"]
DEPENDENCIES = ["climate", "uart"]

ballu_ac_ns = cg.esphome_ns.namespace("ballu_ac")
BalluAcClimate = ballu_ac_ns.class_(
    "BalluAcClimate", climate.Climate, uart.UARTDevice, cg.PollingComponent
)

CONFIG_SCHEMA = (
    climate.climate_schema(BalluAcClimate)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
