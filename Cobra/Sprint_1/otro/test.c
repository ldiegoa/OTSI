TEST_F(matrix_pv_network_ports, pv_network_ports_status_get) {
  int e_status          = 0;
  uint32_t invalid_port = (rand_boolean()) ?
                          (pvtest_rand.get_rnd() % 2) :
                          (1 + MAX_FRONT_PLANE_PORTS +
                           (pvtest_rand.get_rnd() % 2));

  printf(" *** valor del puerto es %d ***", invalid_port);

  struct pv_vport invalid_vport = {};
  struct pv_network_ports_status status = {};
  struct pv_vport first_port_failed = {};

  /* Check1 Network Ports initialized and test OUT bounds parameters (not in
     range 2-57) for Pacuare */
  e_status = pv_vports_bitmap_vport_remove(vport, &bitmap);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_uninitialize(node_id);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_vports_phys_port_init(chassis_id, node_id, invalid_port, &vport);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_vports_bitmap_vport_add(vport, &bitmap);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_initialize(node_id);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_init(node_id, bitmap, &first_port_failed);
  EXPECT_EQ(-EINVAL, e_status);

  e_status = pv_network_ports_status_get(node_id, vport, &status);
  EXPECT_EQ(-EINVAL, e_status);

  /* Check2 Network Ports initialized and get NULL parameters */
  e_status = pv_vports_bitmap_vport_remove(vport, &bitmap);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_uninitialize(node_id);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_vports_phys_port_init(chassis_id, node_id, port_id, &vport);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_initialize(node_id);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_vports_bitmap_vport_add(vport, &bitmap);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_init(node_id, bitmap, &first_port_failed);
  EXPECT_EQ(PV_OK,   e_status);

  e_status = pv_network_ports_status_get(node_id, vport, NULL);
  EXPECT_EQ(-EFAULT, e_status);

  /* Check3 Network Ports initialized and do not get A PHYSICAL PORT */
  e_status = pv_vports_bitmap_vport_remove(vport, &bitmap);
  EXPECT_EQ(PV_OK, e_status);

  e_status = pv_network_ports_uninitialize(node_id);
  EXPECT_EQ(PV_OK, e_status);

  e_status = get_non_physical_vport(chassis_id, node_id, port_id, &invalid_vport);
  EXPECT_EQ(PV_OK, e_status);

  e_status = pv_network_ports_initialize(node_id);
  EXPECT_EQ(PV_OK, e_status);

  e_status = pv_network_ports_init(node_id, bitmap, &first_port_failed);
  EXPECT_EQ(PV_OK, e_status);

  e_status =
    pv_network_ports_status_get(node_id, invalid_vport, &status);
  EXPECT_EQ(-EINVAL, e_status);

  /* Check4 Network Ports initialized and network ports status are
     successfully retrieved. */
  e_status = pv_vports_bitmap_vport_remove(vport, &bitmap);
  EXPECT_EQ(PV_OK,                  e_status);

  e_status = pv_network_ports_uninitialize(node_id);
  EXPECT_EQ(PV_OK,                  e_status);

  e_status = pv_vports_phys_port_init(chassis_id, node_id, port_id, &vport);
  EXPECT_EQ(PV_OK,                  e_status);

  e_status = pv_vports_bitmap_vport_add(vport, &bitmap);
  EXPECT_EQ(PV_OK,                  e_status);

  e_status = pv_network_ports_initialize(node_id);
  EXPECT_EQ(PV_OK,                  e_status);

  e_status = pv_network_ports_init(node_id, bitmap, &first_port_failed);
  EXPECT_EQ(PV_OK,                  e_status);

  e_status = pv_network_ports_status_get(node_id, vport, &status);
  EXPECT_EQ(PV_FEATURE_UNAVAILABLE, e_status);
}
