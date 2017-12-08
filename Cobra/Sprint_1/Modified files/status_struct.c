struct pv_network_ports_status
{
  /** If true indicates that the tx path of the port is enabled */
  bool tx_enabled;							?	***	

  /** If true indicates that the rx path of the port is enabled */		***
  bool rx_enabled;							?	

  /** Indicates port's selected operational speed */				***
  enum pv_port_speed speed;						>	pvi_network_ports_pacuare_speed_set()		***

  /** Specifies port's maximum transfer unit in bytes */		
  uint32_t max_frame_size;						>	pvi_network_ports_pacuare_max_frame_size_set() 	***
				MAC.MAXLEN. max_len 
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




0xF8000000 MAC_1G_MAC_PORT_CONFIG


10G/1G SFP+

The ports use two MACs each, one for each speed. Each MAC has its own register set, and only one can be active at any given time.

1G/100M/10M QSGMII

1G SGMII Conduit

Pacuare has one 1G/100M/10M dedicated port for communication with the Master CPU


The MAC register space is divided in 128KB blocks, one for each MAC

Each block is shared with the CIPP, being the first 64KB assigned to the MAC.






