// mcpwm_gpio_sync_src_config_t tripleHighSyncSourceSetup = {
//     .group_id = highSideGroup,
//     .gpio_num = sacrificialUniversalGpio,
//     .flags = {
//         .pull_down =1
//     }
// };
//write 1 to tripleHighGPIO to update timerPeriod
// *(SET_SUG_REGISTER) |= (1<<(uint32_t)(tripleHighSyncSourceSetup.group_id));
// *(CLEAR_SUG_REGISTER) |= (1<<(uint32_t)(tripleHighSyncSourceSetup.group_id));

// mcpwm_sync_handle_t tripleHighSyncSource; //CONTROLS ALL 3 HIGH TIMERS
// mcpwm_timer_sync_phase_config_t tripleHighOnSync = { //config to use sync to sync timers
//     .sync_src = tripleHighSyncSource, //assign to a sync src
//     .count_value = 600, //assign phase
//     .direction = MCPWM_TIMER_DIRECTION_UP,
// };


/*
    **** ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event());
    **** mcpwm_timer_register_event_callbacks() : call befor timer enable
    **** mcpwm_generator_set_action_on_sync_event()- sync base trigger event,  MCPWM_GEN_SYNC_EVENT_ACTION

mcpwm_capture_channel_config_t triggerChannelSetup = {
    .gpio_num = captureGPIO,
    .intr_priority = 0,
    .prescale = 2,
    .flags = {
    .pos_edge = 1,
    .neg_edge = 0,
    .pull_up = 1,
    .pull_down = 0,
    .invert_cap_signal = 0,
    .io_loop_back = 0 ,
    .keep_io_conf_at_exit =1,
};
mcpwm_cap_channel_handle_t triggerChannelHandle;
mcpwm_capture_timer_sync_phase_config_t ;
mcpwm_capture_timer_config_t triggerSetup = {
    .group_id = id,
    .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    .resolution_hz = 1052631,
};
mcpwm_cap_timer_handle_t triggerHandle;

mcpwm_capture_channel_enable()
mcpwm_capture_channel_register_event_callbacks(),
mcpwm_capture_channel_disable()
mcpwm_capture_timer_enable() 
mcpwm_capture_timer_start() //START
//  mcpwm_carrier_config_t
*/
