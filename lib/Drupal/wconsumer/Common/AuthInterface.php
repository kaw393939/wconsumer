<?php
/**
 * @file
 * Web Consumer Authentication Interface
 */
namespace Drupal\wconsumer\Common;

/**
 * Authentication Interface
 *
 * Define the schema required to build out authentication methods.
 *
 * @package wconsumer
 * @subpackage request
 */
interface AuthInterface {
  public function isInitialized($type);
  public function signRequest($client);
  public function authenticate($user);
  public function logout($user);
  public function onCallback(&$user, $values);
}
