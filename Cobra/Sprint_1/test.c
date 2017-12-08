/*
 * Test setting/getting protocol VID of a port and a protocol
 */
TEST_F(pv_vlans_matrix, port_protocol_vid_set_get)
{
  int status = 0;

  /*
   * TODO: Add port 2 and ports from 6 to 9 when network_ports_speed_set
   * functionality is fully implemented for them
   */
  uint32_t port_number = pvtest_rand.get_rnd_min_max(MIN_FP_1G_PORTS,
                                                     MAX_PORT_TEST);
  uint32_t another_port_number = port_number;
  uint32_t invalid_port_number = 0;
  uint32_t vlan_id             = pvtest_rand.get_rnd_min_max(1,
                                                             pvtest_rand.get_rnd() %
                                                             PV_MAX_VID_NUMBER);
  uint32_t another_vlan_id = pvtest_rand.get_rnd_min_max(1,
                                                         pvtest_rand.get_rnd() %
                                                         PV_MAX_VID_NUMBER);
  uint32_t vid              = 0;
  uint16_t invalid_protocol = 0;
  uint16_t valid_protocol   =
    CIPP_RECOGNIZABLE_PROTOCOLS[pvtest_rand.get_rnd() %
                                CIPP_RECOGNIZABLE_PROTO_MAX_NUMBER];

  struct pv_vport first_port_failed = {};
  struct pv_vport phys_vport, another_phys_vport, invalid_phys_vport,
                  non_phys_vport, bitmap;
  struct pv_vport vport[MAX_PORT_TEST];

  bool repeated_proto = true;

  pv_port_speed speed_selected =
    (pv_port_speed)((pvtest_rand.get_rnd() % PV_PORT_SPEED_1G) +
                    PV_PORT_SPEED_10M);

  // Generates a protocol not included in recognizable CIPP ethertypes
  while (repeated_proto) {
    invalid_protocol = pvtest_rand.get_rnd() % 0xFFFF;
    repeated_proto   = false;

    for (uint32_t i = 0; i < CIPP_RECOGNIZABLE_PROTO_MAX_NUMBER; i++) {
      if (invalid_protocol == CIPP_RECOGNIZABLE_PROTOCOLS[i]) {
        repeated_proto = true;
        break;
      }
    }
  }

  // Initialize the ports
  for (uint32_t i = 0; i < MAX_PORT_TEST; ++i)
  {
    status =
      pv_vports_phys_port_init(CHASSIS_ID, NODE_ID, i,
                               &vport[i]);
    EXPECT_EQ(0, status);

    status = pvi_vports_mgr_vport_create(NODE_ID, vport[i]);
    EXPECT_EQ(0, status);
  }

  // Generates an invalid port: recirc, DMA.
  while (XL_PORT_40G == invalid_port_number) {
    invalid_port_number = pvtest_rand.get_rnd_min_max(0,
                                                      MAX_RECIRC_PORT);
  }

  /* Check 1- Test NULL parameters for vid_set and vid_get */
  status = pv_vlans_port_default_vid_get(NODE_ID,
                                         phys_vport,
                                         NULL);
  EXPECT_EQ(PV_PARM, status);

  status = pv_vlans_protocol_vid_get(NODE_ID,
                                     valid_protocol,
                                     phys_vport,
                                     NULL);
  EXPECT_EQ(PV_PARM, status);

  // Create a non phys port
  status = pv_vports_lag_init(0, &non_phys_vport);
  EXPECT_EQ(PV_OK, status);

  // Create a phys port
  status =
    pv_vports_phys_port_init(CHASSIS_ID, NODE_ID, port_number, &phys_vport);
  EXPECT_EQ(PV_OK, status);

  // Create an invalid phys port
  status =
    pv_vports_phys_port_init(CHASSIS_ID,
                             NODE_ID,
                             invalid_port_number,
                             &invalid_phys_vport);
  EXPECT_EQ(PV_OK, status);

  /* Check 2 -Test not initialized features &  invalid parameters */

  status = pv_vlans_port_default_vid_set(NODE_ID, vlan_id, phys_vport);
  EXPECT_EQ(PV_FEATURE_NOT_INITIALIZED, status);

  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     vlan_id,
                                     valid_protocol,
                                     phys_vport);
  EXPECT_EQ(PV_FEATURE_NOT_INITIALIZED, status);

  status = pv_vlans_protocol_vid_get(NODE_ID,
                                     valid_protocol,
                                     phys_vport,
                                     &vid);
  EXPECT_EQ(PV_FEATURE_NOT_INITIALIZED, status);

  // Initialize the VLANS FLI
  status = pv_vlans_initialize(NODE_ID);
  EXPECT_EQ(PV_OK, status);

  // Set the default VID of a vport that is not a phys/bitmap
  status = pv_vlans_port_default_vid_set(NODE_ID, vlan_id, non_phys_vport);
  EXPECT_EQ(PV_INVALID_PORT, status);

  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     vlan_id,
                                     valid_protocol,
                                     non_phys_vport);
  EXPECT_EQ(PV_INVALID_PORT, status);

  // Set the default VID of a vport with a different node ID
  status = pv_vlans_port_default_vid_set(NODE_ID + 1, vlan_id, non_phys_vport);
  EXPECT_EQ(PV_NOT_INITIALIZED, status);

  status = pv_vlans_protocol_vid_set(NODE_ID + 1,
                                     vlan_id,
                                     valid_protocol,
                                     phys_vport);
  EXPECT_EQ(PV_NOT_INITIALIZED, status);

  // Try to set an invalid VID number to a port and a protocol
  status = pv_vlans_port_default_vid_set(NODE_ID,
                                         PV_MAX_VID_NUMBER + vlan_id,
                                         phys_vport);
  EXPECT_EQ(PV_OUT_OF_RANGE, status);

  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     PV_MAX_VID_NUMBER + vlan_id,
                                     valid_protocol,
                                     phys_vport);
  EXPECT_EQ(PV_OUT_OF_RANGE, status);

  // Try to set a valid VID number to an invalid protocol
  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     vlan_id,
                                     invalid_protocol,
                                     phys_vport);
  EXPECT_EQ(PV_INVALID_PROTOCOL, status);

  // Set the default VID of a phys port that is in reset
  status = pv_vlans_port_default_vid_set(NODE_ID, vlan_id, phys_vport);
  EXPECT_EQ(PV_BUSY, status);

  // Set the default VID of the protocol in a port on reset
  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     vlan_id,
                                     valid_protocol,
                                     phys_vport);
  EXPECT_EQ(PV_BUSY, status);

  /* Check 3- Test setting a VID to a phys port */

  // Speed is set in order to un-reset the port
  status = pv_network_ports_init(NODE_ID, phys_vport, &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_speed_set(NODE_ID,
                                      phys_vport,
                                      speed_selected,
                                      &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_link_enable(NODE_ID, phys_vport,
                                        true, &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  pv_network_ports_enable(NODE_ID, phys_vport,
                          true, &first_port_failed);

  // Set the default VID of a phys port
  status = pv_vlans_port_default_vid_set(NODE_ID, vlan_id, phys_vport);
  EXPECT_EQ(PV_OK, status);

  // Get the default VID of the port
  status = pv_vlans_port_default_vid_get(NODE_ID, phys_vport, &vid);
  EXPECT_EQ(PV_OK, status);

  // Compare the VIDs and check they are equal
  EXPECT_EQ(vid,   vlan_id);

  // Set the default VID of a invalid phys port(RECIRC, DMA)
  status = pv_vlans_port_default_vid_set(NODE_ID, vlan_id, invalid_phys_vport);
  EXPECT_EQ(PV_INVALID_PORT, status);

  // Test setting a VID to a phys port for a valid protocol
  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     vlan_id,
                                     valid_protocol,
                                     phys_vport);
  EXPECT_EQ(PV_OK, status);

  // Get the default VID of the protocol
  status = pv_vlans_protocol_vid_get(NODE_ID,
                                     valid_protocol,
                                     phys_vport,
                                     &vid);
  EXPECT_EQ(PV_OK, status);

  // Compare the VIDs and check they are equal
  EXPECT_EQ(vid,   vlan_id);

  /* Test setting a VID to an invalid phys port for a valid protocol */
  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     vlan_id,
                                     valid_protocol,
                                     invalid_phys_vport);
  EXPECT_EQ(PV_INVALID_PORT, status);


  /*  Test setting a VID to a bitmap */

  // Create a bitmap
  status = pv_vports_bitmap_init(CHASSIS_ID, NODE_ID, &bitmap);
  EXPECT_EQ(PV_OK, status);

  // Add the phys port to the bitmap
  status = pv_vports_bitmap_vport_add(phys_vport, &bitmap);
  EXPECT_EQ(PV_OK, status);

  // Create another phys_port
  while (another_port_number == port_number)
  {
    another_port_number = pvtest_rand.get_rnd_min_max(MIN_FP_1G_PORTS,
                                                      MAX_PORT_TEST);
  }

  status = pv_vports_phys_port_init(CHASSIS_ID,
                                    NODE_ID,
                                    another_port_number,
                                    &another_phys_vport);
  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_init(NODE_ID, another_phys_vport, &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_speed_set(NODE_ID,
                                      another_phys_vport,
                                      speed_selected,
                                      &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  status = pv_network_ports_link_enable(NODE_ID, another_phys_vport,
                                        true, &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  pv_network_ports_enable(NODE_ID, another_phys_vport,
                          true, &first_port_failed);

  // Add the phys_port to the bitmap
  status = pv_vports_bitmap_vport_add(another_phys_vport, &bitmap);
  EXPECT_EQ(PV_OK, status);

  // Set a VID for both ports 
  status = pv_vlans_port_default_vid_set(NODE_ID, another_vlan_id, bitmap);
  EXPECT_EQ(PV_OK, status);

  // Get the default VID of both ports
  status = pv_vlans_port_default_vid_get(NODE_ID, phys_vport, &vid);
  EXPECT_EQ(PV_OK, status);

  // Compare the VIDs and check they are equal
  EXPECT_EQ(vid,   another_vlan_id);

  /* Check 6 - Set a VID protocol for both ports */

  // Set the default VID of the bitmap
  status = pv_vlans_protocol_vid_set(NODE_ID,
                                     another_vlan_id,
                                     valid_protocol,
                                     bitmap);
  EXPECT_EQ(PV_OK, status);

  // Get the default VID of both ports
  status = pv_vlans_protocol_vid_get(NODE_ID,
                                     valid_protocol,
                                     phys_vport,
                                     &vid);
  EXPECT_EQ(PV_OK, status);

  status = pv_vlans_protocol_vid_get(NODE_ID,
                                     valid_protocol,
                                     another_phys_vport,
                                     &vid);
  EXPECT_EQ(PV_OK, status);

  // Compare the VIDs and check they are equal
  EXPECT_EQ(vid,   another_vlan_id);

  /* Check 7 - Remove the default VID of a port & a protocol */

  // Remove port's VID (set it to 0)
  status = pv_vlans_port_default_vid_set(NODE_ID, 0, bitmap);
  EXPECT_EQ(PV_OK, status);

  // Get the default vid from the port where vid has been removed
  status = pv_vlans_port_default_vid_get(NODE_ID, phys_vport, &vid);
  EXPECT_EQ(PV_INVALID_PORT, status);

  // For a particular protocol Set it to 0 with invalid port
  status = pv_vlans_protocol_vid_set(NODE_ID, 0, valid_protocol, non_phys_vport);
  EXPECT_EQ(PV_INVALID_PORT, status);

  // to a valid port
  status = pv_vlans_protocol_vid_set(NODE_ID, 0, valid_protocol, bitmap);
  EXPECT_EQ(PV_OK, status);

  // Get the default VID of the port removed
  status = pv_vlans_protocol_vid_get(NODE_ID,
                                     valid_protocol,
                                     phys_vport,
                                     &vid);
  EXPECT_EQ(PV_OK, status);

  /* Check 8 - Uninitialize the ports */

  // link disable and link down in order to reach network ports uninitialization
  pv_network_ports_enable(NODE_ID, phys_vport,
                          false, &first_port_failed);

  status = pv_network_ports_link_enable(NODE_ID, phys_vport,
                                        false, &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  pv_network_ports_enable(NODE_ID, another_phys_vport,
                          false, &first_port_failed);

  status = pv_network_ports_link_enable(NODE_ID, another_phys_vport,
                                        false, &first_port_failed);
  EXPECT_EQ(PV_OK, status);

  status = pv_vlans_uninitialize(NODE_ID);
  EXPECT_EQ(PV_OK, status);
}
