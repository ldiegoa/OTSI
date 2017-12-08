
  int e_status           = 0;
  uint32_t port_id       = 0;
  uint32_t status_debug  = 0;
  bool     loopback_mode = 0;

  enum pvi_ucx_mac_eee_mode   eee_mode          = PVI_UCX_MAC_EEE_MODE_MAX;
  pv_profile_param_name_str_t status_debug_name = "NP_STATUS_DEBUG_MODE";

  pvi_logtrace_log_trace(PV_LOGTRACE_LOGGING_LEVEL_DEBUG,
                         "status_get",
                         "Fender network ports driver status get",
                         &node_id);

  if (extended_mode) return PV_FEATURE_UNAVAILABLE;

  e_status = pvi_ucx_profile_mgr_int_param_get(node_id,
                                               PVI_PROFILE_CONFIG_TYPE_USER_CONF,
                                               status_debug_name,
                                               &status_debug);
  if (e_status) return e_status;

  e_status = pv_vports_port_get(port, &port_id);
  if (e_status) return e_status;

  if ((extended_mode && (FRONT_PLANE_PORTS_EXT > port_id)) ||
      (!extended_mode && (FRONT_PLANE_PORTS > port_id))) {
    // Link status is retrieved from hardware due to its volatility
    e_status = pvi_network_ports_fender_driver_link_status_get(node_id,
                                                               priv,
                                                               port,
                                                               extended_mode,
                                                               status->speed,
                                                               &status->link);
    if (e_status) return e_status;

    // If debug mode is not set, don't overwrite the software state
    if (0 == status_debug) return e_status;

    e_status = pvi_ucx_mac_mgr_status_get(node_id, port_id, speed,
                                          &status->tx_enabled,
                                          &status->rx_enabled);
    if (e_status) return e_status;

    e_status = pvi_ucx_mac_mgr_mtu_get(node_id,
                                       port_id,
                                       speed,
                                       &status->max_frame_size);
    if (e_status) return e_status;

    e_status = pvi_ucx_mac_mgr_fc_mode_get(node_id, port_id, speed,
                                           &status->fc_mode);
    if (e_status) return e_status;

    /* TODO Mapping between MAC EEE modes and network ports EEE modes.
       Wake timer should be retrieved? */
    e_status = pvi_ucx_mac_mgr_eee_mode_get(node_id, port_id, speed, &eee_mode);
    if (e_status) return e_status;

    e_status = pvi_ucx_mac_mgr_loopback_mode_get(node_id, port_id, speed,
                                                 &loopback_mode);
    if (e_status) return e_status;

    status->loopback_mode = (true == loopback_mode) ? PV_PORT_LOOPBACK_MODE_MAC :
                            status->loopback_mode;
  }

  return e_status;


///////////////////////////////////////////////////////////////////////////////

static int pvi_network_ports_matrix_driver_status_get(
  uint32_t                        node_id,
  void                           *priv,
  struct pv_vport                 port,
  enum pv_port_speed              speed,
  struct pv_network_ports_status *status)
{
  int e_status           = 0;
  uint32_t port_id       = 0;
  uint32_t status_debug  = 0;
  //bool     loopback_mode = 0;

  //enum pvi_ucx_mac_eee_mode   eee_mode          = PVI_UCX_MAC_EEE_MODE_MAX;
  pv_profile_param_name_str_t status_debug_name = "NP_STATUS_DEBUG_MODE";

  e_status = pvi_ucx_profile_mgr_int_param_get(node_id,
                                               PVI_PROFILE_CONFIG_TYPE_USER_CONF,
                                               status_debug_name,
                                               &status_debug);

  if (e_status) return e_status;

  e_status = pv_vports_port_get(port, &port_id);
  if (e_status) return e_status;

  if (FRONT_PLANE_PORTS > port_id) {
    // Link status is retrieved from hardware due to its volatility
    e_status = pvi_network_ports_matrix_driver_link_status_get(node_id,
                                                               priv,
                                                               port,
                                                               status->speed,
                                                               &status->link);
  if (e_status != PV_FEATURE_UNAVAILABLE) return e_status;

    // If debug mode is not set, don't overwrite the software state
  if (0 == status_debug) return e_status;
  }

  return PV_FEATURE_UNAVAILABLE;
}
