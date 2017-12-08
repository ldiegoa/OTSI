int pvi_network_ports_mgr_status_get(
  uint32_t                        node_id,
  struct pv_vport                 port,
  struct pv_network_ports_status *status)
{
  int e_status     = 0;
  uint32_t port_id = 0;
  bool     exists  = false;

  struct pvi_network_ports_mgr_driver_instance  *driver_instance = NULL;
  struct pvi_network_ports_mgr_driver_interface *interface       = NULL;
  struct pvi_network_ports_port_info port_info                   = { { 0 } };

  if (NULL == status) return -EFAULT;

  // Get node instance structure
  e_status = pvi_network_ports_mgr_node_instance_get(node_id,
                                                     &driver_instance);
  if (e_status) return e_status;

  interface = driver_instance->interface;

  // Check that status_get function is not NULL
  if (NULL == interface->ops->status_get) return -ENOSYS;

  e_status = pv_vports_port_get(port, &port_id);
  if (e_status) return e_status;

  e_status = pvi_vports_mgr_port_exists(node_id, port, &exists);
  if (e_status) return e_status;

  if (!exists) return -EINVAL;

  // Retrieve the port status from the software state stored in DB
  e_status = pvi_network_ports_mgr_db_port_info_get(node_id,
                                                    port_id,
                                                    &port_info);
  if (e_status) return e_status;
  

  //**************** FOR TEST DEBUG:
  port_info.status.speed         = PV_PORT_SPEED_1G;
  //**************** 

  memcpy(status, &port_info.status, sizeof(struct pv_network_ports_status));

  if (PV_PORT_SPEED_MAX <= port_info.status.speed) return e_status;

  // The port status can be overwritten by the hardware state in the ASIC
  e_status = interface->ops->status_get(node_id,
                                        driver_instance->priv,
                                        port,
                                        false,
                                        port_info.status.speed,
                                        status);
  return e_status;
}
