<?php

/**
 * @file
 *   drush integration for modernizr.
 */

// The development version of Modernizr can be located here.
define('MODERNIZR_DOWNLOAD_DEV','http://www.modernizr.com/downloads/modernizr-2.5.3.js');


/**
 * Implementation of hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and
 * description.
 *
 * Notice how this structure closely resembles how
 * you define menu hooks.
 *
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function modernizr_drush_command() {
  $items = array();

  $items['modernizr-download-dev'] = array(
    'callback' => '_modernizr_drush_download_dev',
    'description' => dt('(mdl) Downloads the dev version of Modernizr from @dev', array('@dev' => MODERNIZR_DOWNLOAD_DEV)),
    'aliases' => array('mdl'),
    'arguments' => array(
      'path' => dt('Optional. A destination for the Modernizr library. If omitted Drush will use one of the default locations.'),
    ),
  );
  $items['modernizr-custom-build'] = array(
    'callback'    => '_modernizr_drush_custom_build',
    'description' => dt('(mcb) Requests a custom build of Modernizr based on your Drupal module settings. Depends on node.js/npm.'),
    'aliases'     => array('mcb'),
    'arguments'   => array(),
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function modernizr_drush_help($section) {
  switch ($section) {
    case 'drush:modernizr-download-dev':
      return dt("Downloads the dev version of Modernizr from @dev. Accepts an optional destination argument.", array('@dev' => MODERNIZR_DOWNLOAD_DEV));
    case 'drush:modernizr-custom-build':
      return dt('Queries Drupal modules for any Modernizr tests they require, and creates a request to a node.js-powered CLI builder. You must install node.js, npm, and the CLI builder beforehand.');
  }
}

/**
 * Helper function downloads the uncompressed development copy of Modernizr.
 */
function _modernizr_drush_download_dev() {
  $args = func_get_args();
  if ($args[0]) {
    $path = $args[0];
  }
  else {
    $path = drush_get_context('DRUSH_DRUPAL_ROOT');
    if (module_exists('libraries')) {
      $path .= '/' . libraries_get_path('modernizr');
    }
    else {
      $path .= '/' . drupal_get_path('module', 'modernizr') . '/js';
    }
  }

  // Create the path if it does not exist yet.
  if (!is_dir($path)) {
    drush_mkdir($path);
  }

  if (is_dir($path . '/arc')) {
    drush_log('Modernizr already present. No download required.', 'ok');
  }
  elseif (drush_op('chdir', $path) &&
    drush_shell_exec('wget '. MODERNIZR_DOWNLOAD_DEV)) {
      drush_log(dt('The latest Modernizr dev library has been downloaded to @path', array('@path' => $path)), 'success');
  }
  else {
    drush_log(dt('Drush was unable to download Modernizr dev to @path', array('@path' => $path)), 'error');
  }
}

/**
 * Implements drush_MODULE_post_COMMAND().
 */
function drush_modernizr_post_pm_enable() {
  $extensions = func_get_args();
  // Deal with comma delimited extension list.
  if (strpos($extensions[0], ',') !== FALSE) {
    $extensions = explode(',', $extensions[0]);
  }

  if (in_array('modernizr', $extensions) && !drush_get_option('skip')) {
    _modernizr_drush_download_dev();
  }
}

/**
 * Helper function that generates a list of Modernizr tests
 * from other modules and sends them to the node.js CLI builder.
 */
function _modernizr_drush_custom_build() {
  $args = array();
  $tests = modernizr_api_list();
  $output = '';

  // Fetch all the tests, and record which modules supplied them.
  foreach($tests as $test => $info) {
    $args[$info['type']][] = $test;
    $modules[] = $info['module'];
  }

  // Remove duplicate module names.
  $modules = array_unique($modules);

  // Collapse tests into the format accepted by the node builder.
  $tests  = (isset($args['tests']))  ? ' -t '. implode(' ',$args['tests'])  : '';
  $extras = (isset($args['extras'])) ? ' -e '. implode(' ',$args['extras']) : '';
  $groups = (isset($args['groups'])) ? ' -g '. implode(' ',$args['groups']) : '';
  $not    = (isset($args['not']))    ? ' -n '. implode(' ',$args['not'])    : '';

  $output = $tests . $extras . $groups . $not;

  return $output;
}
