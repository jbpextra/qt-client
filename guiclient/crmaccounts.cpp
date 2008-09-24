/*
 * Common Public Attribution License Version 1.0. 
 * 
 * The contents of this file are subject to the Common Public Attribution 
 * License Version 1.0 (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License 
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla 
 * Public License Version 1.1 but Sections 14 and 15 have been added to 
 * cover use of software over a computer network and provide for limited 
 * attribution for the Original Developer. In addition, Exhibit A has 
 * been modified to be consistent with Exhibit B.
 * 
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License. 
 * 
 * The Original Code is xTuple ERP: PostBooks Edition 
 * 
 * The Original Developer is not the Initial Developer and is __________. 
 * If left blank, the Original Developer is the Initial Developer. 
 * The Initial Developer of the Original Code is OpenMFG, LLC, 
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright 
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
 * 
 * Contributor(s): ______________________.
 * 
 * Alternatively, the contents of this file may be used under the terms 
 * of the xTuple End-User License Agreeement (the xTuple License), in which 
 * case the provisions of the xTuple License are applicable instead of 
 * those above.  If you wish to allow use of your version of this file only 
 * under the terms of the xTuple License and not to allow others to use 
 * your version of this file under the CPAL, indicate your decision by 
 * deleting the provisions above and replace them with the notice and other 
 * provisions required by the xTuple License. If you do not delete the 
 * provisions above, a recipient may use your version of this file under 
 * either the CPAL or the xTuple License.
 * 
 * EXHIBIT B.  Attribution Information
 * 
 * Attribution Copyright Notice: 
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
 * 
 * Attribution Phrase: 
 * Powered by xTuple ERP: PostBooks Edition
 * 
 * Attribution URL: www.xtuple.org 
 * (to be included in the "Community" menu of the application if possible)
 * 
 * Graphic Image as provided in the Covered Code, if any. 
 * (online at www.xtuple.com/poweredby)
 * 
 * Display of Attribution Information is required in Larger Works which 
 * are defined in the CPAL as a work which combines Covered Code or 
 * portions thereof with code not governed by the terms of the CPAL.
 */

#include "crmaccounts.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>
#include <metasql.h>

#include "crmaccount.h"
#include "storedProcErrorLookup.h"

crmaccounts::crmaccounts(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
    setupUi(this);

    connect(_print,	 SIGNAL(clicked()),	this,	SLOT(sPrint()));
    connect(_new,	 SIGNAL(clicked()),	this,	SLOT(sNew()));
    connect(_edit,	 SIGNAL(clicked()),	this,	SLOT(sEdit()));
    connect(_view,	 SIGNAL(clicked()),	this,	SLOT(sView()));
    connect(_delete,	 SIGNAL(clicked()),	this,	SLOT(sDelete()));
    connect(_activeOnly, SIGNAL(toggled(bool)),	this,	SLOT(sFillList()));
    connect(_crmaccount, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*)),
				      this, SLOT(sPopulateMenu(QMenu*)));
    connect(omfgThis, SIGNAL(crmAccountsUpdated(int)), this, SLOT(sFillList()));
    connect(omfgThis, SIGNAL(customersUpdated(int, bool)), this, SLOT(sFillList()));
    connect(omfgThis, SIGNAL(prospectsUpdated()), this, SLOT(sFillList()));
    connect(omfgThis, SIGNAL(taxAuthsUpdated(int)), this, SLOT(sFillList()));
    connect(omfgThis, SIGNAL(vendorsUpdated()), this, SLOT(sFillList()));

    _crmaccount->addColumn(tr("Number"),    80, Qt::AlignLeft,  true, "crmacct_number");
    _crmaccount->addColumn(tr("Name"),	    -1, Qt::AlignLeft,  true, "crmacct_name");
    _crmaccount->addColumn(tr("Customer"),  70, Qt::AlignCenter,true, "cust");
    _crmaccount->addColumn(tr("Prospect"),  70, Qt::AlignCenter,true, "prospect");
    _crmaccount->addColumn(tr("Vendor"),    70, Qt::AlignCenter,true, "vend");
    _crmaccount->addColumn(tr("Competitor"),70, Qt::AlignCenter,true, "competitor");
    _crmaccount->addColumn(tr("Partner"),   70, Qt::AlignCenter,true, "partner");
    _crmaccount->addColumn(tr("Tax Auth."), 70, Qt::AlignCenter,true, "taxauth");

    if (_privileges->check("MaintainCRMAccounts"))
    {
      connect(_crmaccount, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
      connect(_crmaccount, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
      connect(_crmaccount, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
    }
    else
    {
      _new->setEnabled(FALSE);
      connect(_crmaccount, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
    }

    _activeOnly->setChecked(true);

    sFillList();
}

crmaccounts::~crmaccounts()
{
    // no need to delete child widgets, Qt does it all for us
}

void crmaccounts::languageChange()
{
    retranslateUi(this);
}

void crmaccounts::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  crmaccount* newdlg = new crmaccount();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccounts::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("crmacct_id", _crmaccount->id());

  crmaccount* newdlg = new crmaccount();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccounts::sDelete()
{
  q.prepare("SELECT deleteCRMAccount(:crmacct_id) AS returnVal;");
  q.bindValue(":crmacct_id", _crmaccount->id());
  q.exec();
  if (q.first())
  {
    int returnVal = q.value("returnVal").toInt();
    if (returnVal < 0)
    {
      systemError(this, storedProcErrorLookup("deleteCRMAccount", returnVal),
		  __FILE__, __LINE__);
      return;
    }
  }
  else if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  sFillList();
}

void crmaccounts::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("crmacct_id", _crmaccount->id());

  crmaccount* newdlg = new crmaccount();
  newdlg->set(params);
  omfgThis->handleNewWindow(newdlg);
}

void crmaccounts::sFillList()
{
  QString sql("SELECT crmacct_id, crmacct_number, crmacct_name,"
	      "     CASE WHEN crmacct_cust_id IS NULL THEN '' ELSE 'Y' END AS cust,"
	      "     CASE WHEN crmacct_prospect_id IS NULL THEN '' ELSE 'Y' END AS prospect,"
	      "     CASE WHEN crmacct_vend_id IS NULL THEN '' ELSE 'Y' END AS vend,"
	      "     CASE WHEN crmacct_competitor_id IS NULL THEN '' ELSE 'Y' END AS competitor,"
	      "     CASE WHEN crmacct_partner_id IS NULL THEN '' ELSE 'Y' END AS partner,"
	      "     CASE WHEN crmacct_taxauth_id IS NULL THEN '' ELSE 'Y' END AS taxauth "
              "FROM crmacct "
	      "<? if exists(\"activeOnly\") ?> WHERE crmacct_active <? endif ?>"
              "ORDER BY crmacct_number;");
  ParameterList params;
  if (! setParams(params))
    return;
  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  _crmaccount->populate(q);
  if (q.lastError().type() != QSqlError::None)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void crmaccounts::sPopulateMenu( QMenu * pMenu )
{
  int menuItem;

  menuItem = pMenu->insertItem(tr("Edit..."), this, SLOT(sEdit()), 0);
  if (!_privileges->check("MaintainCRMAccounts"))
    pMenu->setItemEnabled(menuItem, FALSE);

  pMenu->insertItem(tr("View..."), this, SLOT(sView()), 0);

  menuItem = pMenu->insertItem(tr("Delete"), this, SLOT(sDelete()), 0);
  if (!_privileges->check("MaintainCRMAccounts"))
    pMenu->setItemEnabled(menuItem, FALSE);
}

bool crmaccounts::setParams(ParameterList &params)
{
  if (_activeOnly->isChecked())
    params.append("activeOnly");

  return true;
}

void crmaccounts::sPrint()
{
  ParameterList params;
  if (! setParams(params))
    return;

  orReport report("CRMAccountMasterList", params);
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}
