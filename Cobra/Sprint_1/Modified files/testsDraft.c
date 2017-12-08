 * Check1 node is not attached
 * Check2 vport structure is not created
 * Check3 FLI is already initialized

int pv_network_ports_status_get(
  uint32_t                        node_id,
  struct pv_vport                 port,
  struct pv_network_ports_status *status)
{

  int  e_status     = 0;
  bool is_phys_port = false;

  if (NULL == status) return -EFAULT;

  e_status = pv_vports_phys_port_type_check(port,
                                            &is_phys_port);
  if (e_status) return e_status;

  if (!is_phys_port) return -EINVAL;

  e_status = pvi_network_ports_mgr_status_get(node_id,
                                              port,
                                              status);
  return e_status;
}


* Check4 Structure members set:


struct pv_network_ports_status
{
  /** If true indicates that the tx path of the port is enabled */
  bool tx_enabled;							?		

  /** If true indicates that the rx path of the port is enabled */
  bool rx_enabled;							?	

  /** Indicates port's selected operational speed */
  enum pv_port_speed speed;						>	pvi_network_ports_pacuare_speed_set()	

  /** Specifies port's maximum transfer unit in bytes */		
  uint32_t max_frame_size;						>	pvi_network_ports_pacuare_max_frame_size_set()

  /** Specifies flow control mode */
  enum pv_port_fc_mode fc_mode;						>	pvi_network_ports_pacuare_flow_ctrl_mode_set()

  /** Specifies energy efficient ethernet mode */
  enum pv_network_ports_eee_mode eee_mode;				>	pvi_network_ports_pacuare_eee_config_set()

  /** Wake timer in case it's configurable */
  uint32_t wake_timer;							?

  /** If true indicates that link is up, false if link is down */
  bool link;								>	pvi_network_ports_pacuare_link_enable()

  /** Specifies loopback mode */
  enum pv_port_loopback_mode loopback_mode;				>	pv_network_ports_loopback_mode_set()	

  /** Autonegotiation feature mask */
  uint32_t an_feat_mask;						?

  /** If true indicates that autonegotiation is enabled, false if disabled */
  bool an_enabled;							>	pvi_network_ports_pacuare_autoneg_enable()


		
